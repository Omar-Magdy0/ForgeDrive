#include "PmsmControlCore.h"
#include "math/qmath.h"
#include <cmath>
#include <cstdint>
// Fixed-point filter coefficients for current magnitude estimation (max_a_min_b approximation)
constexpr int32_t PosErrStep = INT32_MAX / 3600;
// Speed scaling: convert from Q32.31 delta-angle to Q7.24 angular velocity
void PmsmControlCore::Foc_init() {}
void PmsmControlCore::Foc_onEnter(McMode prev_mct) {
  foc.state.pidCD.Reset();
  foc.state.pidCQ.Reset();
  control.pidPosition.Reset();
  control.pidSpeed.Reset();
  control.posLoopCnt = 0;
  control.elecMode   = ElecMode_Current;
}
// saturate the position error to be a max of 1 rev (for higher position errors, a profiler should exist for stepping)
void PmsmControlCore::Foc_onExit() {}
void PmsmControlCore::Foc_xmcLoop() {
  POS1.xTick_enter(pTicks);
  POS1.get_ThetaOmega(state.theta_Q31, state.omega_Q15, model.polePairs);
  if (control.posLoopCnt == 0) {
    if (control.mechMode == MechMode_Position) {
      int64_t e_temp      = state.thetaSp_Q31 - state.theta_Q31;
      int32_t e_theta_q31 = (e_temp > PosErrStep) ? PosErrStep : (e_temp < -PosErrStep) ? -PosErrStep : e_temp;
      int32_t diff_Omega  = control.pidPosition.Step(e_theta_q31);
      state.omegaSp_Q15   = Fd::SatAdd(state.omegaSp_Q15, diff_Omega);
    } else {
      control.pidPosition.Step(0);
    }
    control.posLoopCnt = conf.posRate - 1;
  }
  control.posLoopCnt--;
  if (control.mechMode == MechMode_Speed || control.mechMode == MechMode_Position) {
    int32_t eOmega_Q15  = (state.omegaSp_Q15 - state.omega_Q15) >> 2;
    int32_t diffTorque  = control.pidSpeed.Step(eOmega_Q15);
    state.eTorqueSp_Q31 = Fd::SatAdd(state.eTorqueSp_Q31, diffTorque);
  } else {
    control.pidSpeed.Step(0);
  }
  // Clamping logic here for Id/Iq and Possibly MPTA // for now just map to Iq no MPTA (TODO)
  // TODO: Ultra optimized fixed point MPTA , precompute cooffetients, max-alpha-min-beta square root, turning it into semi linear
  // Normal Mode, Iq = T/Kt = Iq = G*T
  if (!control.mptaActive) {
    foc.state.qSp_Q31 = (state.eTorqueSp_Q31);
    foc.state.dSp_Q31 = 0;
  } else {
  }
  uint32_t cMag_UQ31 = Fd::MaxaMinb_UQ31(foc.state.qSp_Q31, foc.state.dSp_Q31);
  if (cMag_UQ31 > control.maxCMag_UQ31 && cMag_UQ31 != 0) {
    uint32_t scale    = ((uint64_t)control.maxCMag_UQ31 << 31) / cMag_UQ31;
    foc.state.dSp_Q31 = ((int64_t)foc.state.dSp_Q31 * scale) >> 31;
    foc.state.qSp_Q31 = ((int64_t)foc.state.qSp_Q31 * scale) >> 31;
  }
  POS1.xTick_exit();
}
void PmsmControlCore::Foc_pwmLoop() {
  state.vBus_Q31 = eSyncMeas.svm.vBus_Q31;
  if (state.vBus_Q31 > control.maxVBus_UQ31) state.fault |= (uint32_t)PmsmFault_Ovlo;
  else if (state.vBus_Q31 < control.minVBus_UQ31) state.fault |= (uint32_t)PmsmFault_Uvlo;
  if (state.fault != (uint32_t)PmsmFault_Ok) {
    FdMc3p_WriteFloat(MC3P1);
    POS1.pTick_exit();
    return;
  }
  POS1.pTick_enter(pTicks);
  POS1.get_eTheta(state.eTheta_Q31, model.polePairs);
  int32_t sin, cos;
  int32_t cAlpha_Q31, cBeta_Q31;
  int32_t valpha_q31, vBeta_Q31;
  arm_sin_cos_q31(state.eTheta_Q31, &sin, &cos);
  arm_clarke_q31(eSyncMeas.svm.cU_Q31, eSyncMeas.svm.cV_Q31, &cAlpha_Q31, &cBeta_Q31);
  arm_park_q31(cAlpha_Q31, cBeta_Q31, &foc.state.cD_Q31, &foc.state.cQ_Q31, sin, cos);
  int64_t power_q61 = (((int64_t)foc.state.cD_Q31 * foc.state.vD_Q31) >> 1) + (((int64_t)foc.state.cQ_Q31 * foc.state.vQ_Q31) >> 1);
  state.power_Q31   = (int32_t)(power_q61 >> 30);
  state.cBus_Q31    = (int32_t)((power_q61 / state.vBus_Q31) * 6);
  state.cMag_UQ31   = Fd::MaxaMinb_UQ31(foc.state.qSp_Q31, foc.state.dSp_Q31);
  // if (state.cMag_UQ31 > control.maxCMag_UQ31) state.fault |= (uint32_t)PmsmFault_OC;
  //  if (state.cBus_Q31 > control.max_ibus_uq31 || state.cBus_Q31 < -control.max_ibus_uq31) state.fault |= (uint32_t)PmsmFault_OC;
  if (state.fault != (uint32_t)PmsmFault_Ok) {
    FdMc3p_WriteFloat(MC3P1);
    POS1.pTick_exit();
    return;
  }
  if (control.elecMode == ElecMode_Current) {
    int32_t ed       = (foc.state.dSp_Q31 - foc.state.cD_Q31) >> 2;
    int32_t eq       = (foc.state.qSp_Q31 - foc.state.cQ_Q31) >> 2;
    int32_t deltaD   = foc.state.pidCD.Step(ed);
    int32_t deltaQ   = foc.state.pidCQ.Step(eq);
    foc.state.vD_Q31 = Fd::SatAdd(foc.state.vD_Q31, deltaD);
    foc.state.vQ_Q31 = Fd::SatAdd(foc.state.vQ_Q31, deltaQ);
  } else {
    foc.state.pidCD.Step(0);
    foc.state.pidCQ.Step(0);
    foc.state.vD_Q31 = foc.state.dSp_Q31;
    foc.state.vQ_Q31 = foc.state.qSp_Q31;
  }
  // Circular overmodulation clamping
  // Here overmodulation logic is intended
  uint32_t magV_Q31 = Fd::MaxaMinb_UQ31(foc.state.vD_Q31, foc.state.vQ_Q31);
  uint32_t limV_Q31 = (uint64_t(state.vBus_Q31) * (MC3P1.maxDuty - MC3P1.minDuty) * foc.state.modIdxInvSqrt3_UQ15) >> 30;
  if (magV_Q31 > limV_Q31 && magV_Q31 != 0U) {
    uint32_t scale_Q31 = ((uint64_t)limV_Q31 << 31) / magV_Q31;
    foc.state.vD_Q31   = (int32_t)(((int64_t)foc.state.vD_Q31 * scale_Q31) >> 31);
    foc.state.vQ_Q31   = (int32_t)(((int64_t)foc.state.vQ_Q31 * scale_Q31) >> 31);
  }
  q15_t alpha_Q15, beta_Q15;
  arm_inv_park_q31(foc.state.vD_Q31, foc.state.vQ_Q31, &valpha_q31, &vBeta_Q31, sin, cos);
  alpha_Q15 = (valpha_q31) / (state.vBus_Q31 >> 15);
  beta_Q15  = (vBeta_Q31) / (state.vBus_Q31 >> 15);
  FdMc3p_WriteSvm(MC3P1, alpha_Q15, beta_Q15);
  POS1.pTick_exit();
}