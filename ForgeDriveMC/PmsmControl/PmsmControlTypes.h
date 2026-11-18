#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include "fd/preprocessor.h"
#include "math/qmath.h"


#define DIRECTION_LIST(X) \
    X(Forward,  0)        \
    X(Backward, 1)

#define MECH_MODE_LIST(X) \
    X(SKIP,     0)        \
    X(Torque,   1)        \
    X(Speed,    2)        \
    X(Position, 3)

#define PID_TYPE_LIST(X) \
    X(Id,       0)       \
    X(Iq,       1)       \
    X(Speed,    2)       \
    X(Position, 3)

#define MC_MODE_LIST(X) \
    X(SKIP,    0)       \
    X(Idle,    1)       \
    X(Foc,     2)       \
    X(SComm,   3)

#define ELEC_MODE_LIST(X) \
    X(Voltage, 0)         \
    X(Current, 1)

#define RUN_MODE_LIST(X) \
    X(Override,   0)     \
    X(ClosedLoop, 1)

#define ESTOP_MODE_LIST(X) \
    X(COAST,          0)   \
    X(RAMP,           1)   \
    X(EMERGENCY_PLUG, 2)

#define PMSM_FAULT_LIST(X) \
    X(OK,     0)           \
    X(UVLO,   1 << 0)      \
    X(OVLO,   1 << 1)      \
    X(OLOAD,  1 << 2)      \
    X(CSFAULT,1 << 3)      \
    X(OT,     1 << 4)      \
    X(OC,     1 << 5)

#define PMSM_ERR_LIST(X) \
    X(OK,                                  0)  \
    X(UNKNOWN_ERROR,                       1)  \
    X(NOT_INITIALIZED,                     2)  \
    X(NOT_IMPLEMENTED,                     3)  \
    X(CONFIG_NOT_ALLOWED_MOTOR_RUNNING,   64)  \
    X(CONFIG_BAD,                         65)

#define ID_STAGE_LIST(X) \
    X(IDLE,           0) \
    X(RS_ID,          1) \
    X(LD_ID,          2) \
    X(REST0,          3) \
    X(LQ_ID,          4)

#define ID_SUB_STAGE_LIST(X) \
    X(ESettle_Start,    0)   \
    X(ESettle_Wait,     1)   \
    X(Active_Sampling,  2)

FD_ENUM_STRUCT(Direction,    uint8_t,  DIRECTION_LIST)
FD_ENUM_STRUCT(MechMode,     uint8_t,  MECH_MODE_LIST)
FD_ENUM_STRUCT(PidType,      uint8_t,  PID_TYPE_LIST)
FD_ENUM_STRUCT(MCMode,       uint8_t,  MC_MODE_LIST)
FD_ENUM_STRUCT(ElecMode,     uint8_t,  ELEC_MODE_LIST)
FD_ENUM_STRUCT(RunMode,      uint8_t,  RUN_MODE_LIST)
FD_ENUM_STRUCT(EStopMode,    uint8_t,  ESTOP_MODE_LIST)
FD_ENUM_STRUCT(PMSM_FAULT,   uint32_t, PMSM_FAULT_LIST)
FD_ENUM_STRUCT(PMSM_ERR,     uint8_t,  PMSM_ERR_LIST)
FD_ENUM_STRUCT(IDStage,      uint8_t,  ID_STAGE_LIST)
FD_ENUM_STRUCT(IDSubStage,   uint8_t,  ID_SUB_STAGE_LIST)


struct ProfilePoint2D
{
    float t;
    float V1;
    float V2;
};

struct CoreState
{
    uint32_t fault;
    int32_t Vbus_q31; /** DC bus voltage (V). */
    int32_t Ibus_q31;
    int32_t eTheta_q31; /** Electrical angle (rad) (-Pi to Pi https://arm-software.github.io/CMSIS-DSP/v1.14.0/group__SinCos.html#gae9e4ddebff9d4eb5d0a093e28e0bc504). */
    int64_t theta_q32p31;
    int64_t theta_sp_q32p31;
    int32_t omega_q16p15; /** Mechanical Angular velocity rad per second*/
    int32_t omega_sp_q16p15;
    int32_t iTorque_sp_q31;
};

struct CoreControl
{
    float inv_kt;
    int32_t max_cmag_q31;
    int32_t max_vbus_q31;
    int32_t min_vbus_q31;
    fd::pid_q31 position_pid;
    fd::pid_q31 speed_pid;
    uint8_t pos_loop_counter = 0;
    MCMode mc_mode = MCMode::Idle; /** Current control mode. */
    MechMode mech_mode;                              /** Mechanical control type */
    ElecMode elec_mode;                              /** Current error type. */
    RunMode run_mode;
    bool mpta_active = false;
};

struct FocState
{
    fd::pid_q31 Id_pid = fd::pid_q31(0, 0, 0);
    fd::pid_q31 Iq_pid = fd::pid_q31(0, 0, 0);
    int32_t Vd_q31 = 0;
    int32_t Vq_q31 = 0;
    int32_t Id_q31 = 0;
    int32_t Iq_q31 = 0;
    int32_t ECq_sp_q31 = 0;
    int32_t ECd_sp_q31 = 0;
    int16_t mod_idx_max_q3p12 = (int16_t)((1 * INT16_MAX) >> 3);
};

struct ConfigPwm
{
    uint32_t pwm_Hz;
    uint32_t deadtime_ns;
    float duty_min;
    float duty_max;
};

struct ConfigAnalog
{
    float sync_scale[7][2]; //MC3P_SYNC_CHANNELS_MAX
    bool offset_calibration;
};

struct ConfigSComm
{
    uint8_t oversample_bits;
    uint32_t esettle_min_ticks;
    uint8_t hfi_N = 20;
    float dc_vinj = 2.5;
    float hfi_vinj = 12;
};

struct PmsmModel
{
    float R = 0;            /** Stator resistance (Ohm). */
    float Ld = 0;           /** D-axis inductance (H). */
    float Lq = 0;           /** Q-axis inductance (H). */
    float psi = 2.0 / 3.0;  /** flux Linkage */
    float J = 0;            /** Rotor inertia. */
    float B = 0;            /** Viscous friction coefficient. */
    uint8_t pole_pairs = 1; /** Motor pole pairs (electrical). */
};

struct ConfigGen
{
    uint32_t max_freq = 20'000;
    uint32_t min_freq = 4'000;
    uint32_t max_deadtime_ns = 1000;
    uint32_t min_deadtime_ns = 300;
    float max_cmag = 10;
    float min_vbus = 30;
    float max_vbus = 40;
    uint32_t xTick_freq = 4000;
    uint32_t pos_rate_div = 5;
};

struct Pid
{
    float Kp;
    float Ki;
    float Kd;
};
