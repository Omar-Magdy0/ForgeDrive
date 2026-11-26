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
  int32_t  Vbus_q31; /** DC bus voltage (V). */
  int32_t  Ibus_q31;
  uint32_t imag_uq31;
  int32_t  power_q31;
  int32_t  eTheta_q31; /** Electrical angle (rad) (-Pi to Pi
                          https://arm-software.github.io/CMSIS-DSP/v1.14.0/group__SinCos.html#gae9e4ddebff9d4eb5d0a093e28e0bc504). */
  int64_t theta_q32p31;
  int64_t theta_sp_q32p31;
  int32_t omega_q16p15; /** Mechanical Angular velocity rad per second*/
  int32_t omega_sp_q16p15;
  int32_t iTorque_sp_q31;
};
struct CoreControl {
  float       inv_kt;
  uint32_t    max_imag_uq31;
  uint32_t    max_ibus_uq31;
  uint32_t    max_vbus_uq31;
  uint32_t    min_vbus_uq31;
  fd::pid_q31 position_pid;
  fd::pid_q31 speed_pid;
  uint8_t     pos_loop_counter = 0;
  McMode      mc_mode          = McMode_IDLE; /** Current control mode. */
  MechMode    mech_mode;                      /** Mechanical control type */
  ElecMode    elec_mode;                      /** Current error type. */
  bool        mpta_active = false;
};
constexpr uint16_t InvSqrt3_uq15 = int16_t(0.5773502691896258 * INT16_MAX);
struct FocState {
  fd::pid_q31 Id_pid              = fd::pid_q31(0, 0, 0);
  fd::pid_q31 Iq_pid              = fd::pid_q31(0, 0, 0);
  int32_t     Vd_q31              = 0;
  int32_t     Vq_q31              = 0;
  int32_t     Id_q31              = 0;
  int32_t     Iq_q31              = 0;
  int32_t     ECq_sp_q31          = 0;
  int32_t     ECd_sp_q31          = 0;
  uint16_t    modidxInvSqrt3_uq15 = InvSqrt3_uq15;
};
