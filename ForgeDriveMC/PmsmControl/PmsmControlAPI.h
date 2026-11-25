PmsmErr setSetpoint(float sp, MechMode mech_mode = MechMode_UNSPECIFIED) {
  if (mech_mode != MechMode_UNSPECIFIED) {
    if (mech_mode < MechMode_TORQUE || mech_mode > MechMode_POSITION) { return PmsmErr_CONFIG_BAD; }
    control.mech_mode = mech_mode;
  }
  switch (control.mech_mode) {
  case MechMode_SPEED: state.omega_sp_q16p15 = (int32_t)(sp * (INT16_MAX / PI)); break;
  case MechMode_POSITION: {
    constexpr float scale = (float)(INT32_MAX / PI);
    state.theta_sp_q32p31 = (int64_t)(sp * scale);
    break;
  }
  case MechMode_TORQUE: {
    float iTorque        = CLAMP(torque_to_iTorque(sp, control.inv_kt), -FD_MC3P_CS_SCALE, FD_MC3P_CS_SCALE);
    state.iTorque_sp_q31 = FD_MC3P_FLOAT_TO_CS(iTorque);
    break;
  }
  default: break;
  }
  return PmsmErr_OK;
}
// Configuration
PmsmErr setModel(PmsmModel m) {
  this->model = m;
  return PmsmErr_OK;
}
PmsmErr setControlMode(ControlMode mode) {
  if (mode.mc_mode != McMode_IDLE && control.mc_mode != McMode_IDLE) { return PmsmErr_CONFIG_NOT_ALLOWED_MOTOR_RUNNING; }
  uint32_t mask = fd_atomic_start();
  setControlMode(mode.mc_mode);
  control.mech_mode = mode.mech_mode;
  fd_atomic_end(mask);
  return PmsmErr_OK;
}
PmsmErr setConfigPwm(ConfigPwm cfg) {
  if (control.mc_mode == McMode_SCOMM) { return PmsmErr_CONFIG_NOT_ALLOWED_MOTOR_RUNNING; } // don't allow pwm reconfiguring during self commissioning
  if (cfg.pwm_hz > conf.max_freq || cfg.pwm_hz < conf.min_freq) { return PmsmErr_CONFIG_BAD; }
  if (cfg.deadtime_ns > conf.max_deadtime_ns || cfg.deadtime_ns < conf.min_deadtime_ns) { return PmsmErr_CONFIG_BAD; }
  if (cfg.duty_min < 0 || cfg.duty_min > 1) { return PmsmErr_CONFIG_BAD; }
  if (cfg.duty_min > cfg.duty_max) { return PmsmErr_CONFIG_BAD; }
  uint32_t mask = fd_atomic_start(); // Atomic operation we do this quick
  pwmConfigUpdate(cfg);
  fd_atomic_end(mask);
  return PmsmErr_OK;
}
PmsmErr setConfigAnalog(ConfigAnalog cfg) {
  if (control.mc_mode != McMode_IDLE || MC3P1.mode != FD_MC3P_MODE_NONE) {
    return PmsmErr_CONFIG_NOT_ALLOWED_MOTOR_RUNNING;
  } // don't allow pwm reconfiguring during self commissioning
  float sync_scale[MC3P_SYNC_CHANNELS_MAX][2];
#define SET_SCALE(id, cfg)        \
  sync_scale[id][0] = cfg.offset; \
  sync_scale[id][1] = cfg.gain
  SET_SCALE(FD_MC3P_VSBUS, cfg.vsbus);
  SET_SCALE(FD_MC3P_VSU, cfg.vsu);
  SET_SCALE(FD_MC3P_VSV, cfg.vsv);
  SET_SCALE(FD_MC3P_VSW, cfg.vsw);
  SET_SCALE(FD_MC3P_CSU, cfg.csu_csbus);
  SET_SCALE(FD_MC3P_CSV, cfg.csv);
  SET_SCALE(FD_MC3P_CSW, cfg.csw);
#undef SET_SCALE
  fd_mc3p_set_sync_scale(MC3P1, sync_scale);
  MC3P1.offset_calibration = cfg.offset_calibration;
  return PmsmErr_OK;
}
PmsmErr setPid(Pid pid) {
  pid.Kp        = CLAMP(pid.Kp, -1, 1);
  pid.Ki        = CLAMP(pid.Ki, -1, 1);
  pid.Kd        = CLAMP(pid.Kd, -1, 1);
  uint32_t mask = fd_atomic_start();
  switch (pid.type) {
  case PidType_ID: foc.state.Id_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd); break;
  case PidType_IQ: foc.state.Iq_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd); break;
  case PidType_POSITION: control.position_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd); break;
  case PidType_SPEED: control.speed_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd); break;
  default: break;
  }
  fd_atomic_end(mask);
  return PmsmErr_OK;
}
PmsmErr setConfigSComm(ConfigSComm cfg) {
  if (control.mc_mode != McMode_IDLE) { return PmsmErr_CONFIG_NOT_ALLOWED_MOTOR_RUNNING; }
  if (cfg.dc_vinj <= 0 || cfg.dc_vinj > FD_MC3P_VS_SCALE) { return PmsmErr_CONFIG_BAD; }
  if (cfg.hfi_vinj <= 0 || cfg.hfi_vinj > FD_MC3P_VS_SCALE) { return PmsmErr_CONFIG_BAD; }
  scomm.conf = cfg;
  return PmsmErr_OK;
}
// Getters
ControlMode getControlMode() { return ControlMode{control.mc_mode, control.mech_mode}; }
PmsmModel   getModel() { return model; }
ConfigPwm   getPwm() {
  ConfigPwm cfg;
  cfg.deadtime_ns = MC3P1.config.deadtime_nS;
  cfg.duty_max    = (float)MC3P1.config.duty_max;
  cfg.duty_min    = (float)MC3P1.config.duty_min;
  cfg.pwm_hz      = MC3P1.config.deadtime_nS;
  return cfg;
}
Pid getPid(PidType n) {
  Pid pid;
  switch (n) {
  case PidType_ID: foc.state.Id_pid.getGain(pid.Kp, pid.Ki, pid.Kd); break;
  case PidType_IQ: foc.state.Iq_pid.getGain(pid.Kp, pid.Ki, pid.Kd); break;
  case PidType_POSITION: control.position_pid.getGain(pid.Kp, pid.Ki, pid.Kd); break;
  case PidType_SPEED: control.speed_pid.getGain(pid.Kp, pid.Ki, pid.Kd); break;
  default:
    pid.Kp = INT32_MAX;
    pid.Ki = INT32_MAX;
    pid.Kd = INT32_MAX;
    break;
  }
  return pid;
}
// Status and Error
uint32_t getFault() const { return state.fault; }
PmsmErr  clearFault() {
  state.fault = (uint32_t)PmsmFault_OK;
  return PmsmErr_OK;
}