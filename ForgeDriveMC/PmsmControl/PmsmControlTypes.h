#pragma once
#include "math/qmath.h"
#include <array>
#include <cstddef>
#include <cstdint>
extern "C" {
#include "fdmc/fdmc.pb.h"
}
struct CoreState {
  uint32_t fault;
  int32_t  vBus_Q31; /** DC bus voltage (V). */
  int32_t  cBus_Q31;
  uint32_t cMag_UQ31;
  int32_t  power_Q31;
  int32_t  eTheta_Q31; /** Electrical angle (rad) (-Pi to Pi
                          https://arm-software.github.io/CMSIS-DSP/v1.14.0/group__SinCos.html#gae9e4ddebff9d4eb5d0a093e28e0bc504). */
  int64_t theta_Q31;
  int64_t thetaSp_Q31;
  int32_t omega_Q15; /** Mechanical Angular velocity rad per second*/
  int32_t omegaSp_Q15;
  int32_t eTorqueSp_Q31;
};
struct CoreControl {
  float       invKt;
  uint32_t    maxCMag_UQ31;
  uint32_t    maxCBus_UQ31;
  uint32_t    maxVBus_UQ31;
  uint32_t    minVBus_UQ31;
  Fd::Pid_Q31 pidPosition;
  Fd::Pid_Q31 pidSpeed;
  uint8_t     posLoopCnt = 0;
  McMode      mcMode     = McMode_Idle; /** Current control mode. */
  MechMode    mechMode;                 /** Mechanical control type */
  ElecMode    elecMode;                 /** Current error type. */
  bool        mptaActive = false;
};
constexpr uint16_t InvSqrt3_UQ15 = int16_t(0.5773502691896258 * INT16_MAX);
struct FocState {
  Fd::Pid_Q31 pidCD               = Fd::Pid_Q31(0, 0, 0);
  Fd::Pid_Q31 pidCQ               = Fd::Pid_Q31(0, 0, 0);
  int32_t     vD_Q31              = 0;
  int32_t     vQ_Q31              = 0;
  int32_t     cD_Q31              = 0;
  int32_t     cQ_Q31              = 0;
  int32_t     dSp_Q31             = 0;
  int32_t     qSp_Q31             = 0;
  uint16_t    modIdxInvSqrt3_UQ15 = InvSqrt3_UQ15;
};
