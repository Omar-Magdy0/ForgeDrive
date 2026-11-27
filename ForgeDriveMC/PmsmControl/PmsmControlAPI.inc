PmsmErr setSetpoint(Setpoint sp) {
  if (sp.mech_mode != MechMode_UNSPECIFIED) {
    if (sp.mech_mode < MechMode_TORQUE || sp.mech_mode > MechMode_POSITION) { return PmsmErr_CONFIG_BAD; }
    control.mech_mode = sp.mech_mode;
  }
  switch (control.mech_mode) {
  case MechMode_SPEED: {
    state.omega_sp_q16p15 = (int32_t)(sp.value * (INT16_MAX / PI));
    break;
  }
  case MechMode_POSITION: {
    constexpr float scale = (float)(INT32_MAX / PI);
    state.theta_sp_q32p31 = (int64_t)(sp.value * scale);
    break;
  }
  case MechMode_TORQUE: {
    float iTorque        = fd::Clamp(torque_to_iTorque(sp.value, control.inv_kt), -FDMC3P_CS_SCALE, FDMC3P_CS_SCALE);
    state.iTorque_sp_q31 = FDMC3P_F2CS(iTorque);
    break;
  }
  default: break;
  }
  return PmsmErr_OK;
}
Setpoint getSetpoint() {
  switch (control.mech_mode) {
  case MechMode_SPEED: {
    constexpr float scale = (float)(INT16_MAX / PI);
    return {(float)state.omega_sp_q16p15 / scale, control.mech_mode};
  }
  case MechMode_POSITION: {
    constexpr float scale = (float)(INT32_MAX / PI);
    return {(float)state.theta_sp_q32p31 / scale, control.mech_mode};
  }
  case MechMode_TORQUE: {
    float iTorque = FDMC3P_CS2F(state.iTorque_sp_q31);
    return {iTorque_to_torque(iTorque, control.inv_kt), control.mech_mode};
  }
  default: return {0.0f, control.mech_mode};
  }
}
// Configuration
PmsmErr setModel(PmsmModel m) {
  if (control.mc_mode != McMode_IDLE) { return PmsmErr_CONFIG_NOT_ALLOWED_MOTOR_RUNNING; }
  this->model = m;
  modelUpdate();
  return PmsmErr_OK;
}
PmsmErr setControlMode(ControlMode mode) {
  if (mode.mc_mode != McMode_IDLE && control.mc_mode != McMode_IDLE) { return PmsmErr_CONFIG_NOT_ALLOWED_MOTOR_RUNNING; }
  uint32_t mask = FdDrv_AtomicStart();
  setControlMode(mode.mc_mode);
  control.mech_mode = mode.mech_mode;
  FdDrv_AtomicEnd(mask);
  return PmsmErr_OK;
}
PmsmErr setConfigPwm(ConfigPwm cfg) {
  if (control.mc_mode == McMode_SCOMM) { return PmsmErr_CONFIG_NOT_ALLOWED_MOTOR_RUNNING; } // don't allow pwm reconfiguring during self commissioning
  if (cfg.pwm_hz > conf.max_freq || cfg.pwm_hz < conf.min_freq) { return PmsmErr_CONFIG_BAD; }
  if (cfg.deadtime_ns > conf.max_deadtime_ns || cfg.deadtime_ns < conf.min_deadtime_ns) { return PmsmErr_CONFIG_BAD; }
  if (cfg.duty_min < 0 || cfg.duty_min > 1) { return PmsmErr_CONFIG_BAD; }
  if (cfg.duty_min > cfg.duty_max) { return PmsmErr_CONFIG_BAD; }
  uint32_t mask = FdDrv_AtomicStart(); // Atomic operation we do this quick
  pwmConfigUpdate(cfg);
  FdDrv_AtomicEnd(mask);
  return PmsmErr_OK;
}
PmsmErr setConfigAnalog(ConfigAnalog cfg) {
  auto& mcp3_inst = MC3P1;
  if (control.mc_mode != McMode_IDLE || mcp3_inst.mode != FdMc3pMode_None) {
    return PmsmErr_CONFIG_NOT_ALLOWED_MOTOR_RUNNING;
  } // don't allow pwm reconfiguring during self commissioning
#define SET_SCALE(inst, id, cfg, op)      \
  inst.syncScale[id][0] = op(cfg.offset); \
  inst.syncScale[id][1] = op(cfg.gain);
  SET_SCALE(mcp3_inst, FdMc3pSync_VsBus, (cfg.vsbus), FDMC3P_F2VS);
  SET_SCALE(mcp3_inst, FdMc3pSync_VsU, (cfg.vsu), FDMC3P_F2VS);
  SET_SCALE(mcp3_inst, FdMc3pSync_VsV, (cfg.vsv), FDMC3P_F2VS);
  SET_SCALE(mcp3_inst, FdMc3pSync_VsW, (cfg.vsw), FDMC3P_F2VS);
  SET_SCALE(mcp3_inst, FdMc3pSync_CsU, (cfg.csu_csbus), FDMC3P_F2CS);
  SET_SCALE(mcp3_inst, FdMc3pSync_CsV, (cfg.csv), FDMC3P_F2CS);
  SET_SCALE(mcp3_inst, FdMc3pSync_CsW, (cfg.csw), FDMC3P_F2CS);
#undef SET_SCALE
  mcp3_inst.config.offsetCalibration = cfg.offset_calibration;
  return PmsmErr_OK;
}
ConfigAnalog getConfigAnalog() {
  ConfigAnalog cfg;
  auto&        mcp3_inst = MC3P1;
#define GET_SCALE(inst, id, dst, op)      \
  dst.offset = op(inst.syncScale[id][0]); \
  dst.gain   = op(inst.syncScale[id][1]);
  GET_SCALE(mcp3_inst, FdMc3pSync_VsBus, cfg.vsbus, FDMC3P_VS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_VsU, cfg.vsu, FDMC3P_VS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_VsV, cfg.vsv, FDMC3P_VS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_VsW, cfg.vsw, FDMC3P_VS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_CsU, cfg.csu_csbus, FDMC3P_CS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_CsV, cfg.csv, FDMC3P_CS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_CsW, cfg.csw, FDMC3P_CS2F);
#undef GET_SCALE
  cfg.offset_calibration = mcp3_inst.config.offsetCalibration;
  return cfg;
}
PmsmErr setPid(Pid pid) {
  pid.Kp        = fd::Clamp(pid.Kp, -1, 1);
  pid.Ki        = fd::Clamp(pid.Ki, -1, 1);
  pid.Kd        = fd::Clamp(pid.Kd, -1, 1);
  uint32_t mask = FdDrv_AtomicStart();
  switch (pid.type) {
  case PidType_ID: foc.state.Id_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd); break;
  case PidType_IQ: foc.state.Iq_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd); break;
  case PidType_POSITION: control.position_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd); break;
  case PidType_SPEED: control.speed_pid.gainUpdate(pid.Kp, pid.Ki, pid.Kd); break;
  default: break;
  }
  FdDrv_AtomicEnd(mask);
  return PmsmErr_OK;
}
PmsmErr setConfigScomm(ConfigScomm cfg) {
  if (control.mc_mode != McMode_IDLE) { return PmsmErr_CONFIG_NOT_ALLOWED_MOTOR_RUNNING; }
  if (cfg.dc_vinj <= 0 || cfg.dc_vinj > FDMC3P_VS_SCALE) { return PmsmErr_CONFIG_BAD; }
  if (cfg.hfi_vinj <= 0 || cfg.hfi_vinj > FDMC3P_VS_SCALE) { return PmsmErr_CONFIG_BAD; }
  scomm.conf = cfg;
  return PmsmErr_OK;
}
ControlMode getControlMode() { return {control.mc_mode, control.mech_mode}; }
ConfigScomm getConfigScomm() { return scomm.conf; }
PmsmModel   getModel() { return model; }
ConfigPwm   getConfigPwm() {
  ConfigPwm cfg;
  cfg.deadtime_ns = MC3P1.config.deadtime_ns;
  cfg.duty_max    = (float)MC3P1.config.maxDuty;
  cfg.duty_min    = (float)MC3P1.config.minDuty;
  cfg.pwm_hz      = MC3P1.config.pwm_hz;
  cfg.dtc_active  = MC3P1.config.dtcActive;
  cfg.dtc_thresh  = MC3P1.config.dtcThresh;
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
  pid.type = n;
  return pid;
}
// Status and Error
uint32_t getFault() const { return state.fault; }
PmsmErr  clearFault() {
  state.fault = (uint32_t)PmsmFault_OK;
  return PmsmErr_OK;
}