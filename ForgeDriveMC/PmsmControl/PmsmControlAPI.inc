PmsmErr SetSetpoint(Setpoint sp) {
  if (sp.mechMode != MechMode_Unspecified) {
    if (sp.mechMode < MechMode_Torque || sp.mechMode > MechMode_Position) { return PmsmErr_ConfigBad; }
    control.mechMode = sp.mechMode;
  }
  switch (control.mechMode) {
  case MechMode_Speed: {
    state.omegaSp_Q15 = (int32_t)(sp.val * (INT16_MAX / Fd::Pi));
    break;
  }
  case MechMode_Position: {
    constexpr float scale = (float)(INT32_MAX / Fd::Pi);
    state.thetaSp_Q31     = (int64_t)(sp.val * scale);
    break;
  }
  case MechMode_Torque: {
    float eTorque       = Fd::Clamp(Torque2ETorque(sp.val, control.invKt), -FDMC3P_CS_SCALE, FDMC3P_CS_SCALE);
    state.eTorqueSp_Q31 = FDMC3P_F2CS(eTorque);
    break;
  }
  default: break;
  }
  return PmsmErr_Ok;
}
Setpoint GetSetpoint() {
  switch (control.mechMode) {
  case MechMode_Speed: {
    constexpr float scale = (float)(INT16_MAX / Fd::Pi);
    return {(float)state.omegaSp_Q15 / scale, control.mechMode};
  }
  case MechMode_Position: {
    constexpr float scale = (float)(INT32_MAX / Fd::Pi);
    return {(float)state.thetaSp_Q31 / scale, control.mechMode};
  }
  case MechMode_Torque: {
    float eTorque = FDMC3P_CS2F(state.eTorqueSp_Q31);
    return {eTorque2Torque(eTorque, control.invKt), control.mechMode};
  }
  default: return {0.0f, control.mechMode};
  }
}
// Configuration
PmsmErr SetModel(PmsmModel m) {
  if (control.mcMode != McMode_Idle) { return PmsmErr_ConfigNotAllowed; }
  this->model = m;
  ModelUpdate();
  return PmsmErr_Ok;
}
PmsmErr SetControlMode(ControlMode mode) {
  if (mode.mcMode != McMode_Idle && control.mcMode != McMode_Idle) { return PmsmErr_ConfigNotAllowed; }
  uint32_t mask = FdDrv_AtomicStart();
  SetControlMode(mode.mcMode);
  control.mechMode = mode.mechMode;
  FdDrv_AtomicEnd(mask);
  return PmsmErr_Ok;
}
PmsmErr SetConfigPwm(ConfigPwm cfg) {
  if (control.mcMode == McMode_Scomm) { return PmsmErr_ConfigNotAllowed; } // don't allow pwm reconfiguring during self commissioning
  if (cfg.pwm_hz > conf.maxPwm_hz || cfg.pwm_hz < conf.minPwm_hz) { return PmsmErr_ConfigBad; }
  if (cfg.deadtime_ns > conf.maxDeadtime_ns || cfg.deadtime_ns < conf.minDeadtime_ns) { return PmsmErr_ConfigBad; }
  if (cfg.minDuty < 0 || cfg.minDuty > 1) { return PmsmErr_ConfigBad; }
  if (cfg.minDuty > cfg.maxDuty) { return PmsmErr_ConfigBad; }
  uint32_t mask = FdDrv_AtomicStart(); // Atomic operation we do this quick
  PwmConfigUpdate(cfg);
  FdDrv_AtomicEnd(mask);
  return PmsmErr_Ok;
}
PmsmErr SetConfigAnalog(ConfigAnalog cfg) {
  auto& mcp3_inst = MC3P1;
  if (control.mcMode != McMode_Idle || mcp3_inst.mode != FdMc3pMode_None) {
    return PmsmErr_ConfigNotAllowed;
  } // don't allow pwm reconfiguring during self commissioning
#define SET_SCALE(inst, id, cfg, op)      \
  inst.syncScale[id][0] = op(cfg.offset); \
  inst.syncScale[id][1] = op(cfg.gain);
  SET_SCALE(mcp3_inst, FdMc3pSync_VsBus, (cfg.vsBus), FDMC3P_F2VS);
  SET_SCALE(mcp3_inst, FdMc3pSync_VsU, (cfg.vsU), FDMC3P_F2VS);
  SET_SCALE(mcp3_inst, FdMc3pSync_VsV, (cfg.vsV), FDMC3P_F2VS);
  SET_SCALE(mcp3_inst, FdMc3pSync_VsW, (cfg.vsW), FDMC3P_F2VS);
  SET_SCALE(mcp3_inst, FdMc3pSync_CsU, (cfg.csU_csBus), FDMC3P_F2CS);
  SET_SCALE(mcp3_inst, FdMc3pSync_CsV, (cfg.csV), FDMC3P_F2CS);
  SET_SCALE(mcp3_inst, FdMc3pSync_CsW, (cfg.csW), FDMC3P_F2CS);
#undef SET_SCALE
  mcp3_inst.config.offsetCalib = cfg.offsetCalib;
  return PmsmErr_Ok;
}
ConfigAnalog GetConfigAnalog() {
  ConfigAnalog cfg;
  auto&        mcp3_inst = MC3P1;
#define GET_SCALE(inst, id, dst, op)      \
  dst.offset = op(inst.syncScale[id][0]); \
  dst.gain   = op(inst.syncScale[id][1]);
  GET_SCALE(mcp3_inst, FdMc3pSync_VsBus, cfg.vsBus, FDMC3P_VS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_VsU, cfg.vsU, FDMC3P_VS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_VsV, cfg.vsV, FDMC3P_VS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_VsW, cfg.vsW, FDMC3P_VS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_CsU, cfg.csU_csBus, FDMC3P_CS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_CsV, cfg.csV, FDMC3P_CS2F);
  GET_SCALE(mcp3_inst, FdMc3pSync_CsW, cfg.csW, FDMC3P_CS2F);
#undef GET_SCALE
  cfg.offsetCalib = mcp3_inst.config.offsetCalib;
  return cfg;
}
PmsmErr SetPid(Pid pid) {
  pid.kp        = Fd::Clamp(pid.kp, -1, 1);
  pid.ki        = Fd::Clamp(pid.ki, -1, 1);
  pid.kd        = Fd::Clamp(pid.kd, -1, 1);
  uint32_t mask = FdDrv_AtomicStart();
  switch (pid.type) {
  case PidType_CD: foc.state.pidCD.GainUpdate(pid.kp, pid.ki, pid.kd); break;
  case PidType_CQ: foc.state.pidCQ.GainUpdate(pid.kp, pid.ki, pid.kd); break;
  case PidType_Position: control.pidPosition.GainUpdate(pid.kp, pid.ki, pid.kd); break;
  case PidType_Speed: control.pidSpeed.GainUpdate(pid.kp, pid.ki, pid.kd); break;
  default: break;
  }
  FdDrv_AtomicEnd(mask);
  return PmsmErr_Ok;
}
PmsmErr SetConfigScomm(ConfigScomm cfg) {
  if (control.mcMode != McMode_Idle) { return PmsmErr_ConfigNotAllowed; }
  if (cfg.dcVinj <= 0 || cfg.dcVinj > FDMC3P_VS_SCALE) { return PmsmErr_ConfigBad; }
  if (cfg.hfiVinj <= 0 || cfg.hfiVinj > FDMC3P_VS_SCALE) { return PmsmErr_ConfigBad; }
  scomm.conf = cfg;
  return PmsmErr_Ok;
}
ControlMode GetControlMode() { return {control.mcMode, control.mechMode}; }
ConfigScomm GetConfigScomm() { return scomm.conf; }
PmsmModel   GetModel() { return model; }
ConfigPwm   GetConfigPwm() {
  ConfigPwm cfg;
  cfg.deadtime_ns = MC3P1.config.deadtime_ns;
  cfg.maxDuty     = (float)MC3P1.config.maxDuty;
  cfg.minDuty     = (float)MC3P1.config.minDuty;
  cfg.pwm_hz      = MC3P1.config.pwm_hz;
  cfg.dtcActive   = MC3P1.config.dtcActive;
  cfg.dtcThresh   = MC3P1.config.dtcThresh;
  return cfg;
}
Pid GetPid(PidType n) {
  Pid pid;
  switch (n) {
  case PidType_CD: foc.state.pidCD.GetGain(pid.kp, pid.ki, pid.kd); break;
  case PidType_CQ: foc.state.pidCQ.GetGain(pid.kp, pid.ki, pid.kd); break;
  case PidType_Position: control.pidPosition.GetGain(pid.kp, pid.ki, pid.kd); break;
  case PidType_Speed: control.pidSpeed.GetGain(pid.kp, pid.ki, pid.kd); break;
  default:
    pid.kp = INT32_MAX;
    pid.ki = INT32_MAX;
    pid.kd = INT32_MAX;
    break;
  }
  pid.type = n;
  return pid;
}
// Status and Error
uint32_t GetFault() const { return state.fault; }
PmsmErr  ClearFault() {
  state.fault = (uint32_t)PmsmFault_Ok;
  return PmsmErr_Ok;
}