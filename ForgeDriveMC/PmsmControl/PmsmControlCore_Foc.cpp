#include "PmsmControlCore.h"
#include "math/qmath.h"
// Fixed-point filter coefficients for current magnitude estimation (max_a_min_b approximation)
constexpr q31_t MAXA_MINB_A0 = (127 / 128.0) * INT32_MAX;
constexpr q31_t MAXA_MINB_A1 = (27 / 32.0) * INT32_MAX;
constexpr q31_t MAXA_MINB_B0 = (3 / 16.0) * INT32_MAX;
constexpr q31_t MAXA_MINB_B1 = (71 / 128.0) * INT32_MAX;
constexpr q15_t SQRT3_Q3P12 = ((int32_t)(1.73205080757 * INT16_MAX) >> 3);
constexpr uint32_t POS_MAX_ERROR_STEP = INT32_MAX/3600;

// Speed scaling: convert from Q32.31 delta-angle to Q7.24 angular velocity
constexpr int SPEED_FROM_DELTA_SHIFT = 7;

static inline q31_t maxa_minb_q31(q31_t i1, q31_t i2)
{
    q31_t max, min;
    i1 = i1 < 0 ? -i1 : i1;
    i2 = i2 < 0 ? -i2 : i2;
    if (i1 > i2)
    {
        max = i1;
        min = i2;
    }
    else
    {
        max = i2;
        min = i1;
    }
    q31_t z0 = (((int64_t)MAXA_MINB_A0 * max + (int64_t)MAXA_MINB_B0 * min) >> 31);
    q31_t z1 = (((int64_t)MAXA_MINB_A1 * max + (int64_t)MAXA_MINB_B1 * min) >> 31);
    return z0 > z1 ? z0 : z1;
}
void PmsmControlCore::Foc_init()
{
}
void PmsmControlCore::Foc_onEnter(MCMode prev_mct)
{
    foc.state.Id_pid.reset();
    foc.state.Iq_pid.reset();
    control.position_pid.reset();
    control.speed_pid.reset();
    control.pos_loop_counter = 0;
    control.elec_mode = ElecMode::Current;
    control.run_mode = RunMode::ClosedLoop;
}
void PmsmControlCore::Foc_onExit()
{
}
void PmsmControlCore::Foc_xmcLoop()
{
    POS1.xTick_enter(pTicks);
    POS1.get_ThetaOmega(state.theta_q32p31, state.omega_q16p15, model.pole_pairs);
    if (control.run_mode == RunMode::ClosedLoop)
    { // Closed loop here , Controllers are active
        if (control.pos_loop_counter == 0)
        {
            if (control.mech_mode == MechMode::Position)
            {                  
                int64_t e_temp = state.theta_sp_q32p31 - state.theta_q32p31; // saturate the position error to be a max of 1 rev (for higher position errors, a profiler should exist for stepping)
                int32_t e_theta_q31 = 
                        (e_temp>POS_MAX_ERROR_STEP) ?  POS_MAX_ERROR_STEP:
                        (e_temp<-POS_MAX_ERROR_STEP)? -POS_MAX_ERROR_STEP:
                        e_temp;
                int32_t diff_Omega = control.position_pid.step(e_theta_q31);
                state.omega_sp_q16p15 = fd::sat_add(state.omega_sp_q16p15, diff_Omega);
            }
            else
            {
                control.position_pid.step(0);
            }
            control.pos_loop_counter = conf.pos_rate_div - 1;
        }
        control.pos_loop_counter--;
        if (control.mech_mode == MechMode::Speed || control.mech_mode == MechMode::Position)
        {
            int32_t e_omega_q16p15 = (state.omega_sp_q16p15 - state.omega_q16p15) >> 2;
            int32_t diff_torque = control.speed_pid.step(e_omega_q16p15);
            state.iTorque_sp_q31 = fd::sat_add(state.iTorque_sp_q31, diff_torque);
        }
        else
        {
            control.speed_pid.step(0);
        }
        // Clamping logic here for Id/Iq and Possibly MPTA // for now just map to Iq no MPTA (TODO)
        // TODO: Ultra optimized fixed point MPTA , precompute cooffetients, max-alpha-min-beta square root, turning it into semi linear
        // Normal Mode, Iq = T/Kt = Iq = G*T
        if(!control.mpta_active)
        {
            foc.state.ECq_sp_q31 = (state.iTorque_sp_q31);
            foc.state.ECd_sp_q31 = 0;
        }else{

        }
        int32_t mag_q31 = maxa_minb_q31(foc.state.ECq_sp_q31, foc.state.ECd_sp_q31);
        if(mag_q31 > control.max_cmag_q31)
        {
            q31_t scale = ((int64_t)control.max_cmag_q31 << 31) / mag_q31;
            foc.state.ECd_sp_q31 = ((int64_t)foc.state.ECd_sp_q31 * scale) >> 31;
            foc.state.ECq_sp_q31 = ((int64_t)foc.state.ECq_sp_q31 * scale) >> 31;
        }
    }
    POS1.xTick_exit();
}
void PmsmControlCore::Foc_pwmLoop()
{
    state.Vbus_q31 = mc3p_sync_meas.svm.vbus_q31;
    if(state.Vbus_q31 > control.max_vbus_q31)state.fault |= (uint32_t)PMSM_FAULT::OVLO;
    else if(state.Vbus_q31 < control.min_vbus_q31)state.fault |= (uint32_t)PMSM_FAULT::UVLO;
    if(state.fault != (uint32_t)PMSM_FAULT::OK)
    {
        fd_mc3p_write_float(MC3P1);
        return;
    }
    POS1.pTick_enter(pTicks);
    POS1.get_eTheta(state.eTheta_q31, model.pole_pairs);
    q31_t sin, cos;
    q31_t Ialpha_q31, Ibeta_q31;
    q31_t valpha_q31, vbeta_q31;
    arm_sin_cos_q31(state.eTheta_q31, &sin, &cos);
    arm_clarke_q31(mc3p_sync_meas.svm.cu_q31, mc3p_sync_meas.svm.cv_q31, &Ialpha_q31, &Ibeta_q31);
    arm_park_q31(Ialpha_q31, Ibeta_q31, &foc.state.Id_q31, &foc.state.Iq_q31, sin, cos);
    int64_t temp0 = (((int64_t)foc.state.Id_q31 * foc.state.Vd_q31 + (int64_t)foc.state.Iq_q31 * foc.state.Vq_q31) >> 1) / (state.Vbus_q31);
    state.Ibus_q31 = temp0 * 3;

    if (control.elec_mode == ElecMode::Current)
    {
        int32_t ed = (foc.state.ECd_sp_q31 - foc.state.Id_q31) >> 2;
        int32_t eq = (foc.state.ECq_sp_q31 - foc.state.Iq_q31) >> 2;
        int32_t delta_Vd = foc.state.Id_pid.step(ed);
        int32_t delta_Vq = foc.state.Iq_pid.step(eq);
        foc.state.Vd_q31 = fd::sat_add(foc.state.Vd_q31, delta_Vd);
        foc.state.Vq_q31 = fd::sat_add(foc.state.Vq_q31, delta_Vq);
    }
    else
    {
        foc.state.Id_pid.step(0);
        foc.state.Iq_pid.step(0);
        foc.state.Vd_q31 = foc.state.ECd_sp_q31;
        foc.state.Vq_q31 = foc.state.ECq_sp_q31;
    }

    // Circular overmodulation clamping
    // Here overmodulation logic is intended
    q31_t vmag_q31 = maxa_minb_q31(foc.state.Vd_q31, foc.state.Vq_q31);
    int32_t vbus_lim_q31 = ((int64_t)state.Vbus_q31 * MC3P1.duty_max_q15) >> 15;
    int32_t mod_idx_q3p12 = (((int32_t)(vmag_q31 >> 16) * SQRT3_Q3P12) / (vbus_lim_q31 >> 16));
    if (mod_idx_q3p12 > foc.state.mod_idx_max_q3p12)
    {
        // scale both Vd and Vq down
        foc.state.Vd_q31 = (foc.state.Vd_q31 >> 12) * (((int32_t)foc.state.mod_idx_max_q3p12 << 12) / mod_idx_q3p12);
        foc.state.Vq_q31 = (foc.state.Vq_q31 >> 12) * (((int32_t)foc.state.mod_idx_max_q3p12 << 12) / mod_idx_q3p12);
    }
    q15_t dalpha_q15, dbeta_q15;
    arm_inv_park_q31(foc.state.Vd_q31, foc.state.Vq_q31, &valpha_q31, &vbeta_q31, sin, cos);
    dalpha_q15 = (valpha_q31) / (state.Vbus_q31 >> 15);
    dbeta_q15 = (vbeta_q31) / (state.Vbus_q31 >> 15);
    fd_mc3p_write_svm(MC3P1, dalpha_q15, dbeta_q15);
    POS1.pTick_exit();
}