PMSM_ERR setSetpoint(float sp, MechMode mech_mode = MechMode::SKIP)
{
    if (mech_mode != MechMode::SKIP)
    {
        if (mech_mode < MechMode::Torque || mech_mode > MechMode::Position)
        {
            return PMSM_ERR::CONFIG_BAD;
        }
        control.mech_mode = mech_mode;
    }
    switch (control.mech_mode)
    {
    case MechMode::Speed:
        state.omega_sp_q16p15 = (int32_t)(sp * (INT16_MAX/M_PI));
        break;
    case MechMode::Position:
    {
        constexpr float scale = (float)(INT32_MAX/M_PI);
        state.theta_sp_q32p31 = (int64_t)(sp * scale);
        break;
    }
    case MechMode::Torque:
    {
        float iTorque = CLAMP(torque_to_iTorque(sp, control.inv_kt), -FD_MC3P_CS_SCALE, FD_MC3P_CS_SCALE);
        state.iTorque_sp_q31 = FD_MC3P_FLOAT_TO_CS(iTorque);
        break;
    }
    default:
        break;
    }
    return PMSM_ERR::OK;
}
// Configuration
PMSM_ERR setModel(PmsmModel m){
    this->model = m;
    return PMSM_ERR::OK;
}
PMSM_ERR setControlMode(MCMode mc_mode, MechMode mech_mode)
{
    if (mc_mode != MCMode::Idle && control.mc_mode != MCMode::Idle)
    {
        return PMSM_ERR::CONFIG_NOT_ALLOWED_MOTOR_RUNNING;
    }
    uint32_t mask = fd_atomic_start();
    setControlMode(mc_mode);
    control.mech_mode = mech_mode;
    fd_atomic_end(mask);
    return PMSM_ERR::OK;
}
PMSM_ERR setConfigPwm(ConfigPwm cfg)
{
    if (control.mc_mode == MCMode::SComm)
    {
        return PMSM_ERR::CONFIG_NOT_ALLOWED_MOTOR_RUNNING;
    } // don't allow pwm reconfiguring during self commissioning
    if (cfg.pwm_Hz > conf.max_freq || cfg.pwm_Hz < conf.min_freq)
    {
        return PMSM_ERR::CONFIG_BAD;
    }
    if (cfg.deadtime_ns > conf.max_deadtime_ns || cfg.deadtime_ns < conf.min_deadtime_ns)
    {
        return PMSM_ERR::CONFIG_BAD;
    }
    if (cfg.duty_min < 0 || cfg.duty_min > 1)
    {
        return PMSM_ERR::CONFIG_BAD;
    }
    if (cfg.duty_min > cfg.duty_max)
    {
        return PMSM_ERR::CONFIG_BAD;
    }

    uint32_t mask = fd_atomic_start(); // Atomic operation we do this quick
    pwmConfigUpdate(cfg);
    fd_atomic_end(mask);
    return PMSM_ERR::OK;
}
PMSM_ERR setConfigAnalog(ConfigAnalog cfg)
{
    if (control.mc_mode != MCMode::Idle || MC3P1.mode != FD_MC3P_MODE_NONE)
    {
        return PMSM_ERR::CONFIG_NOT_ALLOWED_MOTOR_RUNNING;
    } // don't allow pwm reconfiguring during self commissioning
    fd_mc3p_set_sync_scale(MC3P1, cfg.sync_scale);
    MC3P1.offset_calibration = cfg.offset_calibration;
    return PMSM_ERR::OK;
}
PMSM_ERR setPid(Pid pid, PidType n)
{
    pid.Kp = CLAMP(pid.Kp, -1, 1);
    pid.Ki = CLAMP(pid.Ki, -1, 1);
    pid.Kd = CLAMP(pid.Kd, -1, 1);
    uint32_t mask = fd_atomic_start();
    switch (n)
    {
    case PidType::Id:
        foc.state.Id_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd);
        break;
    case PidType::Iq:
        foc.state.Iq_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd);
        break;
    case PidType::Position:
        control.position_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd);
        break;
    case PidType::Speed:
        control.speed_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd);
        break;
    default:
        break;
    }
    fd_atomic_end(mask);
    return PMSM_ERR::OK;
}
PMSM_ERR setConfigSComm(ConfigSComm cfg)
{
    if (control.mc_mode != MCMode::Idle)
    {
        return PMSM_ERR::CONFIG_NOT_ALLOWED_MOTOR_RUNNING;
    }
    if (cfg.dc_vinj <= 0 || cfg.dc_vinj > FD_MC3P_VS_SCALE)
    {
        return PMSM_ERR::CONFIG_BAD;
    }
    if (cfg.hfi_vinj <= 0 || cfg.hfi_vinj > FD_MC3P_VS_SCALE)
    {
        return PMSM_ERR::CONFIG_BAD;
    }
    scomm.conf = cfg;
    return PMSM_ERR::OK;
}
// Getters
void getControlMode(MCMode &mc_mode, MechMode &mech_mode){mc_mode = control.mc_mode; mech_mode = control.mech_mode;}
const PmsmModel& getModel(){return model;}

const ConfigPwm getPwm(){
    ConfigPwm cfg;
    cfg.deadtime_ns = MC3P1.config.deadtime_nS;
    cfg.duty_max = (float)MC3P1.config.duty_max;
    cfg.duty_min = (float)MC3P1.config.duty_min;
    cfg.pwm_Hz = MC3P1.config.deadtime_nS;
    return cfg;
}
const Pid getPid(PidType n)
{
    Pid pid;
    switch (n)
       {
       case PidType::Id:
           foc.state.Id_pid.getGain(pid.Kp, pid.Ki, pid.Kd);
           break;
       case PidType::Iq:
           foc.state.Iq_pid.getGain(pid.Kp, pid.Ki, pid.Kd);
           break;
       case PidType::Position:
           control.position_pid.getGain(pid.Kp, pid.Ki, pid.Kd);
           break;
       case PidType::Speed:
           control.speed_pid.getGain(pid.Kp, pid.Ki, pid.Kd);
           break;
       default:
            pid.Kp = INT32_MAX; pid.Ki = INT32_MAX; pid.Kd = INT32_MAX;
           break;
       }
    return pid;
}
// Status and Error
uint32_t getFault() const
{
    return state.fault;
}
PMSM_ERR clearFault()
{
    state.fault = (uint32_t)PMSM_FAULT::OK;
    return PMSM_ERR::OK;
}