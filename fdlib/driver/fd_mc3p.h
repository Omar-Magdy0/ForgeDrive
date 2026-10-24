/**
 * @file fd_mc3p.h
 * @author Omar Magdy
 * @brief STM32F4 MC3P 3phase motor-control driver API.
 *
 * @details
 * This module configures TIM1, ADC1, and DMA2 to provide:
 * - PWM generation for 3-phase bridge control (trap or SVM)
 * - Injected ADC sampling synchronized to PWM for phase/current sensing
 * - Background ADC scanning via DMA for slow-monitoring channels
 *
 * Timing is driven by TIM1 center-aligned PWM. ADC injected conversions are
 * triggered from TIM1 TRGO (CH4 reference), while regular conversions use
 * DMA in circular double-buffer mode.
 */

#ifndef MCADCPWM3P_H
#define MCADCPWM3P_H

#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif


/** @brief Number of synchronized ADC channels for MC3P. */
#define MC3P_SYNC_CHANNELS_MAX 7
#define MC3P_BG_CHANNELS_MAX 8

#define SATURATE(v , min , max)(( v > max)?max: ((v < min)?0:v))
#define Q15_HALF        16384   // 0.5 × 32768
#define Q15_SQRT3_BY_2  28378   // 0.8660254 × 32768


#include <stdbool.h>
#include "fd_drv.h"
#include <math.h>

    typedef enum
    {
        FD_MC3P_CS_NONE               = 0,
        FD_MC3P_CS_TRIPLE_SHUNT       = 1,
        FD_MC3P_CS_DOUBLE_SHUNT       = 2,
        FD_MC3P_CS_SINGLE_SHUNT       = 3,
        FD_MC3P_CS_INLINE             = 4
    } fd_mc3p_cs_t;
    
    typedef enum
    {
        FD_MC3P_SECTOR_FLOAT = 0, /**< Floating state (all MOSFETs OFF) */
        FD_MC3P_SECTOR_TRAP1,     /**< Trapezoidal Sector 1 */
        FD_MC3P_SECTOR_TRAP2,     /**< Trapezoidal Sector 2 */
        FD_MC3P_SECTOR_TRAP3,     /**< Trapezoidal Sector 3 */
        FD_MC3P_SECTOR_TRAP4,     /**< Trapezoidal Sector 4 */
        FD_MC3P_SECTOR_TRAP5,     /**< Trapezoidal Sector 5 */
        FD_MC3P_SECTOR_TRAP6,     /**< Trapezoidal Sector 6 */
        FD_MC3P_SECTOR_SVM1,      /**< SVM Sector 1 */
        FD_MC3P_SECTOR_SVM2,      /**< SVM Sector 2 */
        FD_MC3P_SECTOR_SVM3,      /**< SVM Sector 3 */
        FD_MC3P_SECTOR_SVM4,      /**< SVM Sector 4 */
        FD_MC3P_SECTOR_SVM5,      /**< SVM Sector 5 */
        FD_MC3P_SECTOR_SVM6       /**< SVM Sector 6 */
    } fd_mc3p_sector_t;

    /**
     * @brief Enumeration for motor control modulation modes.
     */
    typedef enum
    {
        FD_MC3P_MODE_NONE = 0,
        FD_MC3P_MODE_TRAP,
        FD_MC3P_MODE_SVM,
        FD_MC3P_MODE_CALIB
    } fd_mc3p_mode_t;

    
/** @brief Check if sector is within Space Vector Modulation range. */
#define IS_SVM_SECTOR(sector) (sector >= FD_MC3P_SECTOR_SVM1 && sector <= FD_MC3P_SECTOR_SVM6)
/** @brief Check if sector is within Trapezoidal range. */
#define IS_TRAP_SECTOR(sector) (sector >= FD_MC3P_SECTOR_TRAP1 && sector <= FD_MC3P_SECTOR_TRAP6)

    /**
     * @brief Enumeration for the state of each motor phase.
     */
    typedef enum
    {
        FD_MC3P_PHASE_FLOAT = 0, /**< Phase is floating */
        FD_MC3P_PHASE_H_PWM,     /**< High-side PWM active */
        FD_MC3P_PHASE_L_PWM,     /**< Low-side PWM active */
        FD_MC3P_PHASE_COMP,      /**< Complementary PWM active */
        FD_MC3P_PHASE_H_ON,      /**< High-side strictly ON */
        FD_MC3P_PHASE_L_ON       /**< Low-side strictly ON */
    } fd_mc3p_phase_state_t;

    typedef struct {
        fd_mc3p_phase_state_t phase_state[3]; // A,B,C: COMP, L_ON, FLOAT
    } mc3p_trap_sector_map_t;
    
    const mc3p_trap_sector_map_t trap_table[6] = {
    {{ mc3p.trap_table }}
    };

    {% if mc3p.dtc is defined %}
    #define FD_MC3P_DTC_ACTIVE
    {% endif %}
    /**
     * @brief Mapping for synchronized ADC measurements.
     */
    typedef enum
    {
        FD_MC3P_VSBUS = 0,           /**< DC Bus Voltage */
        FD_MC3P_VSU,                 /**< Phase U Voltage */
        FD_MC3P_VSV,                 /**< Phase V Voltage */
        FD_MC3P_VSW,                 /**< Phase W Voltage */
        FD_MC3P_CSU,                 /**< Phase U Current */
        FD_MC3P_CSV,                 /**< Phase V Current */
        FD_MC3P_CSW,                 /**< Phase W Current */
        FD_MC3P_CSBUS = FD_MC3P_CSU, /**< DC Bus Current */
    } fd_mc3p_sync_t;

    /**
     * @brief Configuration structure for MC3P PWM and duty limits.
     */
    typedef struct
    {
        uint32_t pwm_Hz;      /**< PWM Frequency in Hertz */
        uint32_t deadtime_nS; /**< Deadtime in Nanoseconds */
        float duty_max;       /**< Maximum allowable duty cycle */
        float duty_min;       /**< Minimum allowable duty cycle */
    } fd_mc3p_config_t;

    /**
     * @brief Main handle structure for the MC3P driver instance.
     */
    typedef struct
    {
        fd_mc3p_config_t config;
        volatile fd_mc3p_mode_t mode;
        volatile fd_mc3p_sector_t sector_last;
        uint32_t timer_max_q15;
        uint16_t duty_max_q15;
        uint16_t duty_min_q15;
        int16_t dutyu_q15;
        int16_t dutyv_q15;
        int16_t dutyw_q15;
        int32_t sync_scale[MC3P_SYNC_CHANNELS_MAX][2];
        uint16_t bg_data[MC3P_BG_CHANNELS_MAX];
        uint8_t bg_channels;
        int32_t rank_scale[4][2];
        bool offset_calibration;
        uint16_t dtc_comp_q15;
        uint8_t dtc_state;
    } fd_mc3p_handle_t;

    /** @brief Data structure for SVM-specific measurements. */
    typedef struct
    {
        int32_t vbus_q31;
        int32_t cu_q31;
        int32_t cv_q31;
        int32_t cw_q31;
    } fd_mc3p_svm_data_t;

    /** @brief Data structure for Trapezoidal-specific measurements. */
    typedef struct
    {
        int32_t vbus_q31;
        int32_t vbemf_q31;
        int32_t cbus_q31;
    } fd_mc3p_trap_data_t;

/* Conversion Macros */
#define FD_MC3P_VS_TO_FLOAT(vs) ((float)(((int64_t)(vs) * FD_MC3P_VS_SCALE) >> 31))
#define FD_MC3P_CS_TO_FLOAT(cs) ((float)(((int64_t)(cs) * FD_MC3P_CS_SCALE) >> 31))
#define FD_MC3P_FLOAT_TO_VS(f) ((int32_t)(((float)(f) / FD_MC3P_VS_SCALE) * INT32_MAX))
#define FD_MC3P_FLOAT_TO_CS(f) ((int32_t)(((float)(f) / FD_MC3P_CS_SCALE) * INT32_MAX))
#define FD_MC3P_VS_SCALE {{mc3p.vs_scale}}
#define FD_MC3P_CS_SCALE {{mc3p.cs_scale}}

{% for inst in mc3p.instances %}
void pv_mc3p_{{inst.name}}_sector_update(fd_mc3p_handle_t *h, fd_mc3p_sector_t sector);
void pv_mc3p_{{inst.name}}_hw_init(fd_mc3p_handle_t *h);
void pv_mc3p_{{inst.name}}_offset_calibration(fd_mc3p_handle_t *h);
static inline fd_mc3p_mode_t fd_mc3p_sector_to_mode(fd_mc3p_sector_t sector)
{
    if (sector >= FD_MC3P_SECTOR_TRAP1 &&
        sector <= FD_MC3P_SECTOR_TRAP6)
        return FD_MC3P_MODE_TRAP;

    if (sector >= FD_MC3P_SECTOR_SVM1 &&
        sector <= FD_MC3P_SECTOR_SVM6)
        return FD_MC3P_MODE_SVM;

    return FD_MC3P_MODE_NONE;
}
static inline void fd_mc3p_{{inst.name}}_init(fd_mc3p_handle_t *h)
{
    h->duty_max_q15 = (uint16_t)((h->config.duty_max) * 0x7FFF);
    h->duty_min_q15 = (uint16_t)((h->config.duty_min) * 0x7FFF);
    //compute deadtme compensation
    #ifdef FD_MC3P_DTC_ACTIVE
    h->dtc_comp_q15 = (uint16_t)(((float)h->config.deadtime_nS*h->config.pwm_Hz/1e9)*INT16_MAX + 0.5);
    #endif
    
    h->sector_last = FD_MC3P_SECTOR_FLOAT;
    h->mode        = FD_MC3P_MODE_NONE;
    pv_mc3p_{{inst.name}}_hw_init(h);
    if(h->offset_calibration)pv_mc3p_{{inst.name}}_offset_calibration(h);
}
void fd_mc3p_{{inst.name}}_reconfigure_pwm(fd_mc3p_handle_t *h);
void fd_mc3p_{{inst.name}}_bg_startConv(fd_mc3p_handle_t *h);
void fd_mc3p_{{inst.name}}_read_sync(fd_mc3p_handle_t *h, void *scanData);
void fd_mc3p_{{inst.name}}_write_phase_state(fd_mc3p_handle_t *h, fd_mc3p_phase_state_t state_u, fd_mc3p_phase_state_t state_v, fd_mc3p_phase_state_t state_w);
void fd_mc3p_{{inst.name}}_write_phase_duty(fd_mc3p_handle_t *h, int16_t duty_u_q15, int16_t duty_v_q15, int16_t duty_w_q15);
static inline void fd_mc3p_{{inst.name}}_write_float(fd_mc3p_handle_t *h)
{    
    fd_mc3p_{{inst.name}}_write_phase_state(h, FD_MC3P_PHASE_FLOAT, FD_MC3P_PHASE_FLOAT, FD_MC3P_PHASE_FLOAT);
    fd_mc3p_{{inst.name}}_write_phase_duty(h, 0, 0, 0);
    pv_mc3p_{{inst.name}}_sector_update(h, FD_MC3P_SECTOR_FLOAT);
}
static inline void fd_mc3p_{{inst.name}}_write_trap(fd_mc3p_handle_t *h, fd_mc3p_sector_t sector, uint16_t duty_q15)
{
    if(sector < FD_MC3P_SECTOR_TRAP1 || sector > FD_MC3P_SECTOR_TRAP6)
        return;

    // Sector update
    if(h->sector_last != sector)
    {
        pv_mc3p_{{inst.name}}_sector_update(h, sector);
    }
    #ifdef FD_MC3P_DTC_ACTIVE
    h->dutyu_q15 = __SSAT((int32_t)duty_q15 + ((h->dtc_state & (1<<3))? h->dtc_comp_q15 : -h->dtc_comp_q15), 16);
    #else
    h->dutyu_q15 = duty_q15;
    #endif

    h->dutyv_q15 = h->dutyu_q15;
    h->dutyw_q15 = h->dutyu_q15;
    fd_mc3p_{{inst.name}}_write_phase_duty(h, h->dutyu_q15, h->dutyv_q15, h->dutyw_q15);
}
static inline void fd_mc3p_{{inst.name}}_write_svm(fd_mc3p_handle_t *h, int16_t alpha_q15, int16_t beta_q15) 
{
    int32_t vmax, vmin, voff;
    int32_t dutyu_q15, dutyv_q15, dutyw_q15;
    /* αβ → phase (normalized Q15) */
    dutyu_q15 = alpha_q15;

    dutyv_q15 = (-(int32_t)Q15_HALF * alpha_q15
         + (int32_t)Q15_SQRT3_BY_2 * beta_q15) >> 15;

    dutyw_q15 = (-(int32_t)Q15_HALF * alpha_q15
         - (int32_t)Q15_SQRT3_BY_2 * beta_q15) >> 15;
    /* SVPWM zero-sequence injection */
    uint8_t b0 = (dutyu_q15 >= 0);
    uint8_t b1 = (dutyv_q15 >= 0);
    uint8_t b2 = (dutyw_q15 >= 0);

    // 3-bit code
    uint8_t code = (b2 << 2) | (b1 << 1) | b0;
    fd_mc3p_sector_t sector;
    switch(code)
    {
        case 0b001: sector = FD_MC3P_SECTOR_SVM1; break;
        case 0b011: sector = FD_MC3P_SECTOR_SVM2; break;
        case 0b010: sector = FD_MC3P_SECTOR_SVM3; break;
        case 0b110: sector = FD_MC3P_SECTOR_SVM4; break;
        case 0b100: sector = FD_MC3P_SECTOR_SVM5; break;
        case 0b101: sector = FD_MC3P_SECTOR_SVM6; break;
        default: sector = FD_MC3P_SECTOR_FLOAT; break; // should not happen
    }
    // Sector update
    if(h->sector_last != sector)
    {
        pv_mc3p_{{inst.name}}_sector_update(h, sector);
    }
    /* SVPWM zero-sequence injection */
    vmax = dutyu_q15;
    if (dutyv_q15 > vmax) vmax = dutyv_q15;
    if (dutyw_q15 > vmax) vmax = dutyw_q15;

    vmin = dutyu_q15;
    if (dutyv_q15 < vmin) vmin = dutyv_q15;
    if (dutyw_q15 < vmin) vmin = dutyw_q15;

    voff = (vmax + vmin) >> 1;

    dutyu_q15 = (dutyu_q15 - voff) + Q15_HALF;
    dutyv_q15 = (dutyv_q15 - voff) + Q15_HALF;
    dutyw_q15 = (dutyw_q15 - voff) + Q15_HALF;
    #ifdef FD_MC3P_DTC_ACTIVE
    h->dutyu_q15 = __SSAT(dutyu_q15 + ((h->dtc_state & (1<<0))? h->dtc_comp_q15 : -h->dtc_comp_q15), 16);
    h->dutyv_q15 = __SSAT(dutyv_q15 + ((h->dtc_state & (1<<1))? h->dtc_comp_q15 : -h->dtc_comp_q15), 16);
    h->dutyw_q15 = __SSAT(dutyw_q15 + ((h->dtc_state & (1<<2))? h->dtc_comp_q15 : -h->dtc_comp_q15), 16);
    #else
    h->dutyu_q15 = dutyu_q15;
    h->dutyv_q15 = dutyv_q15;
    h->dutyw_q15 = dutyw_q15;
    #endif
    fd_mc3p_{{inst.name}}_write_phase_duty(h, h->dutyu_q15, h->dutyv_q15, h->dutyw_q15);
    h->sector_last = sector;
}
__attribute__((weak)) void fd_mc3p_{{inst.name}}_sync_postScanCallback(void);
__attribute__((weak)) void fd_mc3p_{{inst.name}}_bg_postScanCallback(void);
extern fd_mc3p_handle_t {{inst.name}};
{% endfor %}

inline void fd_mc3p_set_sync_scale_impl(fd_mc3p_handle_t *h, const float scales[MC3P_SYNC_CHANNELS_MAX][2])
{
    for (uint8_t i = 0; i < MC3P_SYNC_CHANNELS_MAX; i++)
    {
        float scale = (i >= FD_MC3P_CSU && i <= FD_MC3P_CSW)? FD_MC3P_CS_SCALE : FD_MC3P_VS_SCALE;
        h->sync_scale[i][0] = roundf((DRV.adc_to_V * INT32_MAX)/(scales[i][0] * scale));
        h->sync_scale[i][1] = roundf((scales[i][1])/DRV.adc_to_V);
    }
}
// ==========================================
// UNIFIED COMPILE-TIME DISPATCH MACROS
// ==========================================

#define FD_MC3P_DISPATCH_IMPL(inst, suffix) fd_mc3p_##inst##suffix
#define FD_MC3P_DISPATCH(inst, suffix) FD_MC3P_DISPATCH_IMPL(inst, suffix)

#define fd_mc3p_init(inst) FD_MC3P_DISPATCH(inst, _init)(&(inst))
#define fd_mc3p_reconfigure_pwm(inst) FD_MC3P_DISPATCH(inst, _reconfigure_pwm)(&(inst))
#define fd_mc3p_bg_startConv(inst) FD_MC3P_DISPATCH(inst, _bg_startConv)(&(inst))
#define fd_mc3p_read_sync(inst, data) FD_MC3P_DISPATCH(inst, _read_sync)(&(inst), data)
#define fd_mc3p_write_phase_state(inst, state_u, state_v, state_w) FD_MC3P_DISPATCH(inst, _write_phase_state)(&(inst), state_u, state_v, state_w)
#define fd_mc3p_write_phase_duty(inst, duty_u, duty_v, duty_w) FD_MC3P_DISPATCH(inst, _write_phase_duty)(&(inst), duty_u, duty_v, duty_w)
#define fd_mc3p_write_float(inst) FD_MC3P_DISPATCH(inst, _write_float)(&(inst))
#define fd_mc3p_write_trap(inst, sector, duty) FD_MC3P_DISPATCH(inst, _write_trap)(&(inst), sector, duty)
#define fd_mc3p_write_svm(inst, a, b) FD_MC3P_DISPATCH(inst, _write_svm)(&(inst), a, b)
#define fd_mc3p_set_sync_scale(inst, arg)(fd_mc3p_set_sync_scale_impl(&(inst), arg))



#ifdef __cplusplus
}
#endif

#endif
