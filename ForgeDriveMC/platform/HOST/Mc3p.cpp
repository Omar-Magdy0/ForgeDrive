#include "Mc3p.h"
#include "SILTick.h"
#include "platform.h"
#include "sil.h"
#include "silgui.h"
#define SIL_DEFAULT_VCC 36.0f
// Number of SIL steps to run for each PWM update
//(for better resolution in the scope plots, better simulation quality and for marginally low time constants)
FdMc3pPhaseState MC3P1_phase_state[3];
FdMc3pHandle     MC3P1;
void             postScanMethod() {
  FdMc3p_MC3P1_SyncPostScanCallback();
  sil_input.load_torque = (dummy_load.K * sil.state.omega + dummy_load.B) * sil.state.omega + dummy_load.Tc;
  sil.step(sil_input);
  silgui_updateData();
  silLogger.log(sil, sil_input);
  SILTick::Instance().RegisterEvent((uint64_t)(1e9 / MC3P1.config.pwm_hz), postScanMethod);
}
static void dPsim_dTheta_Sine(double theta_e, double dPsi_dTheta[3]) {
  double amplitude = sil.param.motor_fluxLinkage; // Back EMF amplitude
  dPsi_dTheta[0]   = -amplitude * sin(theta_e);
  dPsi_dTheta[1]   = -amplitude * sin(theta_e - 2 * M_PI / 3);
  dPsi_dTheta[2]   = -amplitude * sin(theta_e + 2 * M_PI / 3);
};
static void dPsim_dTheta_Trapezoidal(double theta_e, double dPsi_dTheta[3]) {
  double amplitude = sil.param.motor_fluxLinkage; // Back EMF amplitude
  dPsi_dTheta[0]   = -amplitude * Sil::trapezoidal_wave(theta_e);
  dPsi_dTheta[1]   = -amplitude * Sil::trapezoidal_wave(theta_e - 2 * M_PI / 3);
  dPsi_dTheta[2]   = -amplitude * Sil::trapezoidal_wave(theta_e + 2 * M_PI / 3);
}
void PrvMc3p_MC3P1_HwInit(FdMc3pHandle* h) {
  sil_input.dt                = 1.0f / h->config.pwm_hz;
  sil_input.load_torque       = 0.00;
  sil_input.vcc               = SIL_DEFAULT_VCC;
  sil.param.dPsim_dTheta      = dPsim_dTheta_Sine;
  sil.param.inv_Ron           = 0.008;
  sil.param.inv_Roff          = 1e6;
  sil.param.motor_pp          = 6;
  sil.param.motor_Rs          = 0.25;
  sil.param.motor_Ls          = 0.0008;
  sil.param.motor_Lm          = 0.0002;
  sil.param.motor_Ms          = sil.param.motor_Ls / 3;
  sil.param.motor_rotorOffset = 0;
  sil.param.motor_fluxLinkage = 0.025;
  sil.param.motor_B           = 1e-2;
  sil.param.motor_J           = 8e-5;
  sil.param.inv_deatime_ns    = h->config.deadtime_ns;
  sil.param.inv_pwm_freq      = h->config.pwm_hz;
  dummy_load.K                = 1.2e-5;
  dummy_load.J                = 1e-3;
  dummy_load.B                = 5e-2;
  dummy_load.Tc               = 0;
  sil.param.load_J            = dummy_load.J;
  h->maxDuty                  = (uint16_t)((h->config.maxDuty) * 0x7FFF);
  h->minDuty                  = (uint16_t)((h->config.minDuty) * 0x7FFF);
  // compute deadtme compensation
  SILTick::Instance().RegisterEvent((uint64_t)(1e9 / h->config.pwm_hz), postScanMethod);
}
void PrvMc3p_MC3P1_SectorUpdate(FdMc3pHandle* h, FdMc3pSector sector) {
  FdMc3pMode cm = FdMc3p_Sector2Mode(sector);
  if (cm != h->mode) // mode changed... call mode update functions
  {
    switch (cm) {
    case FdMc3pMode_Trap: FdMc3p_MC3P1_WritePhaseState(h, FdMc3pPhaseState_Comp, FdMc3pPhaseState_Comp, FdMc3pPhaseState_Comp); break;
    case FdMc3pMode_Svm: FdMc3p_MC3P1_WritePhaseState(h, FdMc3pPhaseState_Comp, FdMc3pPhaseState_Comp, FdMc3pPhaseState_Comp); break;
    default: break;
    }
    h->mode = cm;
  }
  // update sector
  switch (h->mode) {
  case FdMc3pMode_Trap: break;
  case FdMc3pMode_Svm: break;
  default: break;
  }
  h->sectorLast = sector;
}
void PrvMc3p_MC3P1_OffsetCalibration(FdMc3pHandle* h) {}
void FdMc3p_MC3P1_ReconfigurePwm(FdMc3pHandle* h) {
  h->maxDuty = (uint16_t)((h->config.maxDuty) * 0x7FFF);
  h->minDuty = (uint16_t)((h->config.minDuty) * 0x7FFF);
  PrvMc3p_DtcUpdateComp(h);
  sil_input.dt             = 1.0f / h->config.pwm_hz;
  sil.param.inv_pwm_freq   = h->config.pwm_hz;
  sil.param.inv_deatime_ns = h->config.deadtime_ns;
}
void FdMc3p_MC3P1_BgStartConv(FdMc3pHandle* h) {}
void FdMc3p_MC3P1_ReadSync(FdMc3pHandle* h, void* data) {
  uint8_t is_svm  = IS_SVM_SECTOR(h->sectorLast);
  uint8_t is_trap = IS_TRAP_SECTOR(h->sectorLast);
  if (is_svm) {
    // Convert float currents to Q31 format for the control algorithm
    // Scaling: 1.0A = X counts (adjust FD_MC3P_CS_SCALE to tune sensitivity)
    ((FdMc3pSvmData*)(data))->vBus_vsQ31 = FDMC3P_F2VS(sil_input.vcc);
    ((FdMc3pSvmData*)(data))->cU_csQ31   = FDMC3P_F2CS(sil.state.ip[0]);
    ((FdMc3pSvmData*)(data))->cV_csQ31   = FDMC3P_F2CS(sil.state.ip[1]);
    ((FdMc3pSvmData*)(data))->cW_csQ31   = FDMC3P_F2CS(sil.state.ip[2]);
    PrvMc3p_DtcUpdatePolarity(h, ((FdMc3pSvmData*)(data))->cU_csQ31, (1 << 0));
    PrvMc3p_DtcUpdatePolarity(h, ((FdMc3pSvmData*)(data))->cV_csQ31, (1 << 1));
    PrvMc3p_DtcUpdatePolarity(h, ((FdMc3pSvmData*)(data))->cW_csQ31, (1 << 2));
  } else if (is_trap) {
    // Back-EMF would be: Ke * ωe where ωe = pp * ω_mech
    ((FdMc3pSvmData*)(data))->vBus_vsQ31 = FDMC3P_F2VS(sil_input.vcc);
    float adc_syncRank1, adc_syncRank2;
    switch (h->sectorLast) {
    case FdMc3pSector_Trap1:
      adc_syncRank1 = sil.state.vp[2] - sil.state.vn;
      adc_syncRank2 = sil.state.ip[0];
      break;
    case FdMc3pSector_Trap2:
      adc_syncRank1 = sil.state.vp[1];
      adc_syncRank2 = sil.state.ip[0];
      break;
    case FdMc3pSector_Trap3:
      adc_syncRank1 = sil.state.vp[0];
      adc_syncRank2 = sil.state.ip[1];
      break;
    case FdMc3pSector_Trap4:
      adc_syncRank1 = sil.state.vp[2];
      adc_syncRank2 = sil.state.ip[1];
      break;
    case FdMc3pSector_Trap5:
      adc_syncRank1 = sil.state.vp[1];
      adc_syncRank2 = sil.state.ip[2];
      break;
    case FdMc3pSector_Trap6:
      adc_syncRank1 = sil.state.vp[0];
      adc_syncRank2 = sil.state.ip[2];
      break;
    default: break;
    }
    ((FdMc3pTrapData*)(data))->vBemf_vsQ31 = FDMC3P_F2VS(adc_syncRank1);
    ((FdMc3pTrapData*)(data))->cBus_csQ31  = FDMC3P_F2CS(adc_syncRank2);
    PrvMc3p_DtcUpdatePolarity(h, ((FdMc3pTrapData*)(data))->cBus_csQ31, (1 << 3));
  } else {
    ((FdMc3pSvmData*)(data))->vBus_vsQ31 = FDMC3P_F2VS(sil_input.vcc);
  }
}
void FdMc3p_MC3P1_WritePhaseState(FdMc3pHandle* h, FdMc3pPhaseState state_u, FdMc3pPhaseState state_v, FdMc3pPhaseState state_w) {
  sil_input.drive[0]   = (state_u != FdMc3pPhaseState_Float) ? 1 : 0;
  sil_input.drive[1]   = (state_v != FdMc3pPhaseState_Float) ? 1 : 0;
  sil_input.drive[2]   = (state_w != FdMc3pPhaseState_Float) ? 1 : 0;
  MC3P1_phase_state[0] = state_u;
  MC3P1_phase_state[1] = state_v;
  MC3P1_phase_state[2] = state_w;
}
void FdMc3p_MC3P1_WritePhaseDuty(FdMc3pHandle* h, int16_t duty_u_q15, int16_t duty_v_q15, int16_t duty_w_q15) {
  duty_u_q15    = (uint32_t)MC3P_SATURATE(duty_u_q15, h->minDuty, h->maxDuty);
  duty_v_q15    = (uint32_t)MC3P_SATURATE(duty_v_q15, h->minDuty, h->maxDuty);
  duty_w_q15    = (uint32_t)MC3P_SATURATE(duty_w_q15, h->minDuty, h->maxDuty);
  float duty[3] = {((float)duty_u_q15 / INT16_MAX), ((float)duty_v_q15 / INT16_MAX), ((float)duty_w_q15 / INT16_MAX)};
  for (int i = 0; i < 3; i++) {
    if (MC3P1_phase_state[i] == FdMc3pPhaseState_LOn) {
      duty[i] = 0;
    } else if (MC3P1_phase_state[i] == FdMc3pPhaseState_HOn) {
      duty[i] = 1;
    }
    sil_input.duty[i] = duty[i];
  }
}
