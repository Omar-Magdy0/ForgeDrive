#include "PmsmControlCore.h"
#include "fdmc.pb.h"
#include "qmath.h"
PmsmErr PmsmControlCore::init() {
  pTicks = 0;
  xTicks = 0;
  modelUpdate();
  configGenUpdate();
  /* mode initialization*/
  voltage_q31_to_float(state.Vbus_q31);
  current_q31_to_float(state.Ibus_q31);
  angle_q31_to_float(state.eTheta_q31);
  state.fault = PmsmFault_OK;
  SComm_init();
  Foc_init();
  /* hardware initialization */
  POS1.init();
  FdMc3p_Init(MC3P1);
  FdMc3p_WriteFloat(MC3P1);
  return PmsmErr_OK;
};
void PmsmControlCore::configGenUpdate() {
  control.max_imag_uq31 = FDMC3P_F2CS(fd::Clamp(fd::Abs(conf.max_cmag), 0, FDMC3P_CS_SCALE));
  control.min_vbus_uq31 = FDMC3P_F2VS(fd::Clamp(fd::Abs(conf.min_vbus), 0, FDMC3P_VS_SCALE));
  control.max_vbus_uq31 = FDMC3P_F2VS(fd::Clamp(fd::Abs(conf.max_vbus), 0, FDMC3P_VS_SCALE));
  xTicks_period_ns      = 1'000'000'000 / conf.xtick_freq;
}
void PmsmControlCore::modelUpdate() {
  const float kt = model.psi * model.pole_pairs;
  control.inv_kt = (kt != 0.0f) ? (1.0f / kt) : 0.0f;
}
void PmsmControlCore::xmcLoop() {
  switch (control.mc_mode) {
  case McMode_FOC: Foc_xmcLoop(); break;
  case McMode_SCOMM: SComm_xmcLoop(); break;
  default: break;
  }
  xTicks = xTicks + 1;
}
void PmsmControlCore::pwmConfigUpdate(ConfigPwm cfg) {
  pwm_freq_hz              = cfg.pwm_hz;
  pTick_period_ns          = static_cast<uint32_t>(1'000'000'000 / static_cast<float>(pwm_freq_hz));
  MC3P1.config.pwm_hz      = pwm_freq_hz;
  MC3P1.config.deadtime_ns = cfg.deadtime_ns;
  MC3P1.config.minDuty     = cfg.duty_min;
  MC3P1.config.maxDuty     = cfg.duty_max;
  MC3P1.config.dtcThresh   = cfg.dtc_thresh;
  MC3P1.config.dtcActive   = cfg.dtc_active;
  FdMc3p_ReconfigurePwm(MC3P1);
}
void PmsmControlCore::pwmLoop() {
  uint32_t start = FdDrv_DwtTick();
  FdMc3p_ReadSync(MC3P1, &mc3p_sync_meas);
  switch (control.mc_mode) {
  case McMode_IDLE: FdMc3p_WriteFloat(MC3P1); break;
  case McMode_FOC: Foc_pwmLoop(); break;
  case McMode_SCOMM: SComm_pwmLoop(); break;
  default: break;
  }
  pTicks                    = pTicks + 1;
  volatile uint32_t elapsed = FdDrv_DwtTick() - start;
}
void PmsmControlCore::setControlMode(McMode mc_mode) {
  if (control.mc_mode != mc_mode) {
    // Exit current mode
    switch (control.mc_mode) {
    case McMode_FOC: Foc_onExit(); break;
    case McMode_SCOMM: SComm_onExit(); break;
    default: break;
    }
    // Enter new mode
    switch (mc_mode) {
    case McMode_FOC: Foc_onEnter(control.mc_mode); break;
    case McMode_SCOMM: SComm_onEnter(control.mc_mode); break;
    default: break;
    }
    control.mc_mode = mc_mode;
  }
}
