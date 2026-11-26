#include "PmsmControlCore.h"
#include "math/qmath.h"
#include <cmath>
#include <cstdint>
using namespace fd;
// Fixed-point filter coefficients for current magnitude estimation (max_a_min_b approximation)
constexpr uint32_t POS_MAX_ERROR_STEP = INT32_MAX / 3600;
// Speed scaling: convert from Q32.31 delta-angle to Q7.24 angular velocity
void PmsmControlCore::Foc_init() {}
void PmsmControlCore::Foc_onEnter(McMode prev_mct) {
  foc.state.Id_pid.reset();
  foc.state.Iq_pid.reset();
  control.position_pid.reset();
  control.speed_pid.reset();
  control.pos_loop_counter = 0;
  control.elec_mode        = ElecMode_CURRENT;
}
void PmsmControlCore::Foc_onExit() {}
void PmsmControlCore::Foc_xmcLoop() {
  POS1.xTick_enter(pTicks);
  POS1.get_ThetaOmega(state.theta_q32p31, state.omega_q16p15, model.pole_pairs);
  if (control.pos_loop_counter == 0) {
    if (control.mech_mode == MechMode_POSITION) {
      int64_t e_temp = state.theta_sp_q32p31 -
                       state
                           .theta_q32p31; // saturate the position error to be a max of 1 rev (for higher position errors, a profiler should exist for stepping)
      int32_t e_theta_q31   = (e_temp > POS_MAX_ERROR_STEP) ? POS_MAX_ERROR_STEP : (e_temp < -POS_MAX_ERROR_STEP) ? -POS_MAX_ERROR_STEP : e_temp;
      int32_t diff_Omega    = control.position_pid.step(e_theta_q31);
      state.omega_sp_q16p15 = fd::sat_add(state.omega_sp_q16p15, diff_Omega);
    } else {
      control.position_pid.step(0);
    }
    control.pos_loop_counter = conf.pos_rate_div - 1;
  }
  control.pos_loop_counter--;
  if (control.mech_mode == MechMode_SPEED || control.mech_mode == MechMode_POSITION) {
    int32_t e_omega_q16p15 = (state.omega_sp_q16p15 - state.omega_q16p15) >> 2;
    int32_t diff_torque    = control.speed_pid.step(e_omega_q16p15);
    state.iTorque_sp_q31   = fd::sat_add(state.iTorque_sp_q31, diff_torque);
  } else {
    control.speed_pid.step(0);
  }
  // Clamping logic here for Id/Iq and Possibly MPTA // for now just map to Iq no MPTA (TODO)
  // TODO: Ultra optimized fixed point MPTA , precompute cooffetients, max-alpha-min-beta square root, turning it into semi linear
  // Normal Mode, Iq = T/Kt = Iq = G*T
  if (!control.mpta_active) {
    foc.state.ECq_sp_q31 = (state.iTorque_sp_q31);
    foc.state.ECd_sp_q31 = 0;
  } else {
  }
  uint32_t imag_uq31 = maxa_minb_uq31(foc.state.ECq_sp_q31, foc.state.ECd_sp_q31);
  if (imag_uq31 > control.max_imag_uq31 && imag_uq31 != 0) {
    uint32_t scale       = ((uint64_t)control.max_imag_uq31 << 31) / imag_uq31;
    foc.state.ECd_sp_q31 = ((int64_t)foc.state.ECd_sp_q31 * scale) >> 31;
    foc.state.ECq_sp_q31 = ((int64_t)foc.state.ECq_sp_q31 * scale) >> 31;
  }
  POS1.xTick_exit();
}
void PmsmControlCore::Foc_pwmLoop() {
  state.Vbus_q31 = mc3p_sync_meas.svm.vbus_q31;
  if (state.Vbus_q31 > control.max_vbus_uq31) state.fault |= (uint32_t)PmsmFault_OVLO;
  else if (state.Vbus_q31 < control.min_vbus_uq31) state.fault |= (uint32_t)PmsmFault_UVLO;
  POS1.pTick_enter(pTicks);
  POS1.get_eTheta(state.eTheta_q31, model.pole_pairs);
  int32_t sin, cos;
  int32_t Ialpha_q31, Ibeta_q31;
  int32_t valpha_q31, vbeta_q31;
  arm_sin_cos_q31(state.eTheta_q31, &sin, &cos);
  arm_clarke_q31(mc3p_sync_meas.svm.cu_q31, mc3p_sync_meas.svm.cv_q31, &Ialpha_q31, &Ibeta_q31);
  arm_park_q31(Ialpha_q31, Ibeta_q31, &foc.state.Id_q31, &foc.state.Iq_q31, sin, cos);
  int64_t power_q61 = (((int64_t)foc.state.Id_q31 * foc.state.Vd_q31) >> 1) + (((int64_t)foc.state.Iq_q31 * foc.state.Vq_q31) >> 1);
  state.power_q31   = (int32_t)(power_q61 >> 30);
  state.Ibus_q31    = (int32_t)((power_q61 / state.Vbus_q31) * 6);
  state.imag_uq31   = maxa_minb_uq31(foc.state.ECq_sp_q31, foc.state.ECd_sp_q31);
  // if (state.imag_uq31 > control.max_imag_uq31) state.fault |= (uint32_t)PmsmFault_OC;
  //  if (state.Ibus_q31 > control.max_ibus_uq31 || state.Ibus_q31 < -control.max_ibus_uq31) state.fault |= (uint32_t)PmsmFault_OC;
  if (state.fault != (uint32_t)PmsmFault_OK) {
    fd_mc3p_write_float(MC3P1);
    return;
  }
  if (control.elec_mode == ElecMode_CURRENT) {
    int32_t ed       = (foc.state.ECd_sp_q31 - foc.state.Id_q31) >> 2;
    int32_t eq       = (foc.state.ECq_sp_q31 - foc.state.Iq_q31) >> 2;
    int32_t delta_Vd = foc.state.Id_pid.step(ed);
    int32_t delta_Vq = foc.state.Iq_pid.step(eq);
    foc.state.Vd_q31 = fd::sat_add(foc.state.Vd_q31, delta_Vd);
    foc.state.Vq_q31 = fd::sat_add(foc.state.Vq_q31, delta_Vq);
  } else {
    foc.state.Id_pid.step(0);
    foc.state.Iq_pid.step(0);
    foc.state.Vd_q31 = foc.state.ECd_sp_q31;
    foc.state.Vq_q31 = foc.state.ECq_sp_q31;
  }
  // Circular overmodulation clamping
  // Here overmodulation logic is intended
  uint32_t vmag_q31 = maxa_minb_uq31(foc.state.Vd_q31, foc.state.Vq_q31);
  uint32_t vlim_q31 = (uint64_t(state.Vbus_q31) * (MC3P1.duty_max_q15 - MC3P1.duty_min_q15) * foc.state.modidxInvSqrt3_uq15) >> 30;
  if (vmag_q31 > vlim_q31 && vmag_q31 != 0U) {
    uint32_t scale_q31 = ((uint64_t)vlim_q31 << 31) / vmag_q31;
    foc.state.Vd_q31   = (int32_t)(((int64_t)foc.state.Vd_q31 * scale_q31) >> 31);
    foc.state.Vq_q31   = (int32_t)(((int64_t)foc.state.Vq_q31 * scale_q31) >> 31);
  }
  q15_t dalpha_q15, dbeta_q15;
  arm_inv_park_q31(foc.state.Vd_q31, foc.state.Vq_q31, &valpha_q31, &vbeta_q31, sin, cos);
  dalpha_q15 = (valpha_q31) / (state.Vbus_q31 >> 15);
  dbeta_q15  = (vbeta_q31) / (state.Vbus_q31 >> 15);
  fd_mc3p_write_svm(MC3P1, dalpha_q15, dbeta_q15);
  POS1.pTick_exit();
}