#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include "fd_mc3p.h"
#include "math/qmath.h"

enum struct Direction : uint8_t
{
    Forward = 0, /** Forward rotation. */
    Backward = 1 /** Reverse rotation. */
};

enum struct MechMode : uint8_t
{
    SKIP,
    Torque,
    Speed,
    Position
};

enum struct PidType : uint8_t
{
    Id = 0,
    Iq,
    Speed,
    Position
};

enum struct MCMode : uint8_t
{
    SKIP,
    None = 0,
    Idle,
    Fault,
    Foc,  /** Open-loop V/Hz (V/F) commutation. */
    SComm /** Self-commissioning / parameter identification. */
};

enum struct ElecMode : uint8_t
{
    Voltage = 0,
    Current
};

enum struct RunMode : uint8_t
{
    Override,
    ClosedLoop
};

enum struct EStopMode : uint8_t
{
    COAST,
    RAMP,
    EMERGENCY_PLUG
};

enum struct PMSM_FAULT : uint32_t
{
    OK = 0,
    UVLO = 1 << 0,
    OVLO = 1 << 1,
    OLOAD = 1 << 2,
    CSFAULT = 1 << 3,
    OT = 1 << 4,
    OC = 1 << 5
};

enum struct PMSM_ERR : uint8_t
{
    OK = 0,
    UNKNOWN_ERROR = 1,
    NOT_INITIALIZED = 2,
    NOT_IMPLEMENTED = 3,

    CONFIG_NOT_ALLOWED_MOTOR_RUNNING = 64,
    CONFIG_BAD = 65
};

enum struct IDStage : uint8_t
{
    IDLE,
    RS_ID,           /** Stator resistance identification. */
    LD_ID,           /** D-axis inductance identification. */
    REST0,
    LQ_ID            /** Q-axis inductance identification. */
};

enum struct IDSubStage : uint8_t
{
    ESettle_Start,
    ESettle_Wait,
    Active_Sampling
};

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
    float sync_scale[MC3P_SYNC_CHANNELS_MAX][2];
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
