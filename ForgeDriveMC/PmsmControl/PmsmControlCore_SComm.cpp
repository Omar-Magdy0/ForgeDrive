#include "PmsmControlCore.h"
#include <cstdint>
// Builds upon FOC to run, tune and identify parameters
void PmsmControlCore::SComm_init() {}
void PmsmControlCore::SComm_onEnter(McMode prev_mct) {
  scomm.idStage     = IdStage_Res;
  control.elecMode  = ElecMode_Voltage;
  scomm.hfiOmegaRpt = static_cast<int32_t>((scomm.HFI_ANGLE_SCALE / scomm.conf.hfiN) * INT32_MAX);
  scomm.dcVinj_Q31  = FDMC3P_F2VS(scomm.conf.dcVinj);
  scomm.hfiVinj_Q31 = FDMC3P_F2VS(scomm.conf.hfiVinj);
}
void PmsmControlCore::SComm_onExit() {}
void PmsmControlCore::SComm_pwmLoop() {
  int32_t sin, cos;
  // Hfi added logic here......
  switch (scomm.idStage) {
  case IdStage_Res: {
    if (scomm.sampleCnt && scomm.idSub == IdSubStage_ActiveSampling) {
      scomm.accumulate0 += foc.state.cD_Q31;
      scomm.sampleCnt    = scomm.sampleCnt - 1;
    }
    break;
  }
  case IdStage_IndD: {
    arm_sin_cos_q31(scomm.hfiTheta_Q31, &sin, &cos);
    foc.state.dSp_Q31 = ((int64_t)scomm.hfiVinj_Q31 * sin) >> 31;
    foc.state.qSp_Q31 = 0;
    if (scomm.sampleCnt && scomm.idSub == IdSubStage_ActiveSampling) {
      // Heterodyne the read current.....
      int32_t hfiD       = ((int64_t)foc.state.cD_Q31 * sin) >> 31;
      int32_t hfiQ       = ((int64_t)foc.state.cD_Q31 * cos) >> 31;
      scomm.accumulate0 += hfiD;
      scomm.accumulate1 += hfiQ;
      scomm.sampleCnt    = scomm.sampleCnt - 1;
    }
    scomm.hfiTheta_Q31 += scomm.hfiOmegaRpt;
    break;
  }
  case IdStage_IndQ: {
    arm_sin_cos_q31(scomm.hfiTheta_Q31, &sin, &cos);
    foc.state.dSp_Q31 = 0;
    foc.state.qSp_Q31 = ((int64_t)scomm.hfiVinj_Q31 * sin) >> 31;
    if (scomm.sampleCnt && scomm.idSub == IdSubStage_ActiveSampling) {
      // Heterodyne the read current.....
      int32_t hfiD       = ((int64_t)foc.state.cQ_Q31 * sin) >> 31;
      int32_t hfiQ       = ((int64_t)foc.state.cQ_Q31 * cos) >> 31;
      scomm.accumulate0 += hfiD;
      scomm.accumulate1 += hfiQ;
      scomm.sampleCnt    = scomm.sampleCnt - 1;
    }
    scomm.hfiTheta_Q31 += scomm.hfiOmegaRpt;
    break;
  }
  default: break;
  }
  Foc_pwmLoop();
}
void PmsmControlCore::SComm_xmcLoop() {
  // Go step by step into identification process...
  // First Identify DC resistance
  switch (scomm.idStage) {
  case IdStage_Res: {
    // Apply constant dc voltage on d-axis
    // Force angle and speed to 0
    state.eTheta_Q31  = 0;
    foc.state.dSp_Q31 = scomm.dcVinj_Q31;
    foc.state.qSp_Q31 = 0;
    // wait for electrical settling
    switch (scomm.idSub) {
    case IdSubStage_SettleStart:
      scomm.eSettleStartTick = pTicks;
      scomm.idSub            = IdSubStage_SettleWait;
      scomm.accumulate0      = 0;
      scomm.sampleCnt        = 1 << scomm.conf.oversampleBits;
      break;
    case IdSubStage_SettleWait:
      if (pTicks - scomm.eSettleStartTick > scomm.conf.minSettleTicks) { scomm.idSub = IdSubStage_ActiveSampling; }
      break;
    case IdSubStage_ActiveSampling:
      if (scomm.sampleCnt == 0) {
        scomm.model.res_ohm = FDMC3P_VS2F(scomm.dcVinj_Q31) / FDMC3P_CS2F((scomm.accumulate0 >> scomm.conf.oversampleBits));
        scomm.idStage       = IdStage_IndD;
        scomm.idSub         = IdSubStage_SettleStart;
      }
      break;
    default: break;
    }
    break;
  }
  case IdStage_IndD: {
    state.eTheta_Q31 = 0;
    // wait for electrical settling
    switch (scomm.idSub) {
    case IdSubStage_SettleStart:
      scomm.eSettleStartTick = pTicks;
      scomm.idSub            = IdSubStage_SettleWait;
      scomm.accumulate0      = 0;
      scomm.accumulate1      = 0;
      scomm.sampleCnt        = 1 << scomm.conf.oversampleBits;
      break;
    case IdSubStage_SettleWait:
      if (pTicks - scomm.eSettleStartTick > scomm.conf.minSettleTicks) { scomm.idSub = IdSubStage_ActiveSampling; }
      break;
    case IdSubStage_ActiveSampling:
      if (scomm.sampleCnt == 0) {
        int32_t Idd   = scomm.accumulate0 >> scomm.conf.oversampleBits;
        int32_t Idq   = scomm.accumulate1 >> scomm.conf.oversampleBits;
        int32_t Imag2 = (((int64_t)Idd * Idd) + ((int64_t)Idq * Idq)) >> 31;
        int32_t Imag;
        arm_sqrt_q31(Imag2, &Imag);
        float Zd           = vs2f(scomm.hfiVinj_Q31) / (cs2f(2 * Imag));
        float Xd           = sqrt(Zd * Zd - scomm.model.res_ohm * scomm.model.res_ohm);
        scomm.model.indD_h = (Xd) / (theta2f(scomm.hfiOmegaRpt) * pwm_hz);
        scomm.idSub        = IdSubStage_SettleStart;
        scomm.idStage      = IdStage_Rest0;
      }
      break;
    default: break;
    }
  }
  case IdStage_Rest0: {
    switch (scomm.idSub) {
    case IdSubStage_SettleStart:
      scomm.eSettleStartTick = pTicks;
      scomm.idSub            = IdSubStage_SettleWait;
      scomm.accumulate0      = 0;
      scomm.accumulate1      = 0;
      scomm.sampleCnt        = 1 << scomm.conf.oversampleBits;
      state.eTheta_Q31       = 0;
      foc.state.dSp_Q31      = 0;
      foc.state.qSp_Q31      = 0;
      break;
    case IdSubStage_SettleWait:
      if (pTicks - scomm.eSettleStartTick > scomm.conf.minSettleTicks) {
        scomm.idSub   = IdSubStage_SettleStart;
        scomm.idStage = IdStage_IndQ;
      }
      break;
    default: break;
    }
  }
  case IdStage_IndQ: {
    state.eTheta_Q31 = 0;
    // wait for electrical settling
    switch (scomm.idSub) {
    case IdSubStage_SettleStart:
      scomm.eSettleStartTick = pTicks;
      scomm.idSub            = IdSubStage_SettleWait;
      scomm.accumulate0      = 0;
      scomm.accumulate1      = 0;
      scomm.sampleCnt        = 1 << scomm.conf.oversampleBits;
      break;
    case IdSubStage_SettleWait:
      if (pTicks - scomm.eSettleStartTick > scomm.conf.minSettleTicks) {
        scomm.eSettleStartTick = pTicks;
        scomm.idSub            = IdSubStage_ActiveSampling;
      }
      break;
    case IdSubStage_ActiveSampling:
      if (scomm.sampleCnt == 0) {
        int32_t Iqd   = scomm.accumulate0 >> (scomm.conf.oversampleBits);
        int32_t Iqq   = scomm.accumulate1 >> (scomm.conf.oversampleBits);
        int32_t Imag2 = (((int64_t)Iqd * Iqd) + ((int64_t)Iqq * Iqq)) >> 31;
        int32_t Imag;
        arm_sqrt_q31(Imag2, &Imag);
        float Zq           = vs2f(scomm.hfiVinj_Q31) / (cs2f(2 * Imag));
        float Xq           = sqrt(Zq * Zq - scomm.model.res_ohm * scomm.model.res_ohm);
        scomm.model.indQ_h = (Xq) / (theta2f(scomm.hfiOmegaRpt) * pwm_hz);
        scomm.idSub        = IdSubStage_SettleStart;
        scomm.idStage      = IdStage_Res;
      }
      break;
    default: break;
    }
  }
  default: break;
  }
}
