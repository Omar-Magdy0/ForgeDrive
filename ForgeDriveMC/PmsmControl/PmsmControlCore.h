
#pragma once
#include "Drv.h"
#include "Mc3p.h"
#include "PmsmControlTypes.h"
#include "PosDriver.h"
#include "arm_math.h"
#include "qmath.h"
#include <array>
#include <cstddef>
#include <cstdint>
struct PmsmControlCore {
  const ConfigGen        conf;
  volatile uint32_t      xTicks;
  uint32_t               xTicks_period_ns;
  volatile uint32_t      pTicks{};                                                                  /** PWM period in ticks (timer ticks). */
  uint32_t               pwm_freq_hz;                                                               /** PWM carrier frequency (Hz). */
  uint32_t               pTick_period_ns{(uint32_t)(1'000'000'000.0f / static_cast<float>(10000))}; /** PWM tick period (us). */
  constexpr inline float voltage_q31_to_float(q31_t voltage_q31) const { return FDMC3P_VS2F(voltage_q31); }
  constexpr inline float current_q31_to_float(q31_t current_q31) const { return FDMC3P_CS2F(current_q31); }
  constexpr inline float angle_q31_to_float(q31_t eAngle_q31) const { return (float)eAngle_q31 * (PI / INT32_MAX); }
  constexpr inline float torque_to_iTorque(float t, float inv_kt) { return (t * inv_kt); }
  constexpr inline float iTorque_to_torque(float it, float kt) { return (it * kt); }
  inline uint32_t        xTicks_to_us(uint32_t ticks) const { return ((uint64_t)ticks * xTicks_period_ns) / 1000; };
  inline uint32_t        xTicks_to_ms(uint32_t ticks) const { return xTicks_to_us(ticks) / 1000; };
  union {
    FdMc3pSvmData  svm;
    FdMc3pTrapData trap;
  } mc3p_sync_meas{};
  // Control variables
  CoreState   state;
  CoreControl control;
  PmsmModel   model;
#include "PmsmControlCore_Foc.inc"
#include "PmsmControlCore_SComm.inc"
  void setControlMode(McMode mc_mode);
  void pwmConfigUpdate(ConfigPwm cfg);
  void (*onFault)(void* ctx, PmsmFault f);
  PmsmControlCore(void (*_onFault)(void*, PmsmFault), const ConfigGen& _conf) : conf(_conf), onFault(_onFault) {};
  void pwmLoop();
  void xmcLoop();
#include "PmsmControlAPI.inc"
  PmsmErr init();
  void    configGenUpdate();
  void    modelUpdate();
};
