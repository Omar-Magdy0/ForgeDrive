
#include "arm_math.h"
#include "fd_mc3p.h"
#include "fd_hall.h"
#include "fd_drv.h"
#include "math/math.h"
#include <array>
#include <cstdint>
#include <cstddef>
#include "PmsmControlTypes.h"
#include "PosDriver.h"

class PmsmControlCore
{
    const ConfigGen conf;

    volatile uint32_t xTicks;
    uint32_t xTicks_period_ns;
    volatile uint32_t pTicks{};                                                       /** PWM period in ticks (timer ticks). */
    uint32_t pwm_freq_hz;                                                             /** PWM carrier frequency (Hz). */
    uint32_t pTick_period_ns{ (uint32_t)(1'000'000'000.0f / static_cast<float>(10000)) }; /** PWM tick period (us). */
    constexpr inline float voltage_q31_to_float(q31_t voltage_q31) const { return FD_MC3P_VS_TO_FLOAT(voltage_q31); }
    constexpr inline float current_q31_to_float(q31_t current_q31) const { return FD_MC3P_CS_TO_FLOAT(current_q31); }
    constexpr inline float angle_q31_to_float(q31_t eAngle_q31) const { return (float)eAngle_q31 * (M_PI / INT32_MAX);}
    constexpr inline float torque_to_iTorque(float t, float inv_kt){return (t*inv_kt);}
    constexpr inline float iTorque_to_torque(float it, float kt){return (it * kt);}
    inline uint32_t xTicks_to_us(uint32_t ticks)const{return ((uint64_t)ticks*xTicks_period_ns)/1000;};
    inline uint32_t xTicks_to_ms(uint32_t ticks)const{return xTicks_to_us(ticks)/1000;};

    union
    {
        fd_mc3p_svm_data_t svm;
        fd_mc3p_trap_data_t trap;
    } mc3p_sync_meas{};

    // Control variables
    CoreState state;
    CoreControl control;
    PmsmModel model;

#include "PmsmControlCore_Foc.h"
#include "PmsmControlCore_SComm.h"

    void setControlMode(MCMode mc_mode);
    void pwmConfigUpdate(ConfigPwm cfg);
    void (*onFault)(void *ctx, PMSM_FAULT f);
    
    public:
    PmsmControlCore(void(*_onFault)(void*, PMSM_FAULT), const ConfigGen &_conf) : conf(_conf), onFault(_onFault){};
    void pwmLoop();
    void xmcLoop();
#include "PmsmControlAPI.h"
    PMSM_ERR init();
    void configGenUpdate();
    void modelUpdate();
};
