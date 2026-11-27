#include "PmsmControlCore.h"
#include "fdmc.pb.h"
#include "qmath.h"
PmsmErr PmsmControlCore::Init() {
  pTicks = 0;
  xTicks = 0;
  ModelUpdate();
  ConfigGenUpdate();
  /* mode initialization*/
  vs2f(state.vBus_Q31);
  cs2f(state.cBus_Q31);
  theta2f(state.eTheta_Q31);
  state.fault = PmsmFault_Ok;
  SComm_init();
  Foc_init();
  /* hardware initialization */
  POS1.init();
  FdMc3p_Init(MC3P1);
  FdMc3p_WriteFloat(MC3P1);
  return PmsmErr_Ok;
};
void PmsmControlCore::ConfigGenUpdate() {
  control.maxCMag_UQ31 = FDMC3P_F2CS(Fd::Clamp(Fd::Abs(conf.maxCMag), 0, FDMC3P_CS_SCALE));
  control.minVBus_UQ31 = FDMC3P_F2VS(Fd::Clamp(Fd::Abs(conf.minVBus), 0, FDMC3P_VS_SCALE));
  control.maxVBus_UQ31 = FDMC3P_F2VS(Fd::Clamp(Fd::Abs(conf.maxVBus), 0, FDMC3P_VS_SCALE));
  xTicksPeriod_ns      = 1'000'000'000 / conf.xtick_hz;
}
void PmsmControlCore::ModelUpdate() {
  const float kt = model.flux_wb * model.polePairs;
  control.invKt  = (kt != 0.0f) ? (1.0f / kt) : 0.0f;
}
void PmsmControlCore::XmcLoop() {
  switch (control.mcMode) {
  case McMode_Foc: Foc_xmcLoop(); break;
  case McMode_Scomm: SComm_xmcLoop(); break;
  default: break;
  }
  xTicks = xTicks + 1;
}
void PmsmControlCore::PwmConfigUpdate(ConfigPwm cfg) {
  pwm_hz                   = cfg.pwm_hz;
  pTickPeriod_ns           = static_cast<uint32_t>(1'000'000'000 / static_cast<float>(pwm_hz));
  MC3P1.config.pwm_hz      = pwm_hz;
  MC3P1.config.deadtime_ns = cfg.deadtime_ns;
  MC3P1.config.minDuty     = cfg.minDuty;
  MC3P1.config.maxDuty     = cfg.maxDuty;
  MC3P1.config.dtcThresh   = cfg.dtcThresh;
  MC3P1.config.dtcActive   = cfg.dtcActive;
  FdMc3p_ReconfigurePwm(MC3P1);
}
void PmsmControlCore::PwmLoop() {
  uint32_t start = FdDrv_DwtTick();
  FdMc3p_ReadSync(MC3P1, &eSyncMeas);
  switch (control.mcMode) {
  case McMode_Idle: FdMc3p_WriteFloat(MC3P1); break;
  case McMode_Foc: Foc_pwmLoop(); break;
  case McMode_Scomm: SComm_pwmLoop(); break;
  default: break;
  }
  pTicks                    = pTicks + 1;
  volatile uint32_t elapsed = FdDrv_DwtTick() - start;
}
void PmsmControlCore::SetControlMode(McMode mcMode) {
  if (control.mcMode != mcMode) {
    // Exit current mode
    switch (control.mcMode) {
    case McMode_Foc: Foc_onExit(); break;
    case McMode_Scomm: SComm_onExit(); break;
    default: break;
    }
    // Enter new mode
    switch (mcMode) {
    case McMode_Foc: Foc_onEnter(control.mcMode); break;
    case McMode_Scomm: SComm_onEnter(control.mcMode); break;
    default: break;
    }
    control.mcMode = mcMode;
  }
}
