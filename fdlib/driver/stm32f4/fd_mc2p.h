/**
 * @file fd_mc2p.h
 * @author Omar Magdy
 * @brief STM32F4 MC2P 3phase motor-control driver API.
 *
 * @details
 * This module configures TIM1, ADC1, and DMA2 to provide:
 * - PWM generation for 2-phase bridge control
 * - Injected ADC sampling synchronized to PWM for phase/current sensing
 * - Background ADC scanning via DMA for slow-monitoring channels
 *
 * Timing is driven by TIM1 center-aligned PWM. ADC injected conversions are
 * triggered from TIM1 TRGO (CH4 reference), while regular conversions use
 * DMA in circular double-buffer mode.
 */

#ifndef FD_MC2P_H
#define FD_MC2P_H

#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Number of synchronized ADC channels for MC2P. */
#define MC2P_SYNC_CHANNELS 3
#include <stdbool.h>
    typedef enum
    {
        FD_MC2P_SECTOR_FLOAT = 0, /**< Floating state (all MOSFETs OFF) */
    } fd_mc2p_sector_t;

    /**
     * @brief Enumeration for motor control modulation modes.
     */
    typedef enum
    {
        FD_MC2P_MODE_NONE = 0,
        FD_MC2P_MODE_CALIB
    } fd_mc2p_mode_t;

    /**
     * @brief Enumeration for the state of each motor phase.
     */
    typedef enum
    {
        FD_MC2P_PHASE_FLOAT = 0, /**< Phase is floating */
        FD_MC2P_PHASE_H_PWM,     /**< High-side PWM active */
        FD_MC2P_PHASE_L_PWM,     /**< Low-side PWM active */
        FD_MC2P_PHASE_COMP,      /**< Complementary PWM active */
        FD_MC2P_PHASE_H_ON,      /**< High-side strictly ON */
        FD_MC2P_PHASE_L_ON       /**< Low-side strictly ON */
    } fd_mc2p_phase_state_t;

    /**
     * @brief Mapping for synchronized ADC measurements.
     */
    typedef enum
    {
        FD_MC2P_VSBUS = 0,                 /**< DC Bus Voltage */
        FD_MC2P_CSU,                       /**< Phase U Current */
        FD_MC2P_CSV,                       /**< Phase V Current */
        FD_MC2P_CSBUS = FD_MC2P_CSU, /**< DC Bus Current */
    } fd_mc2p_sync_t;

    /**
     * @brief Configuration structure for MC2P PWM and duty limits.
     */
    typedef struct
    {
        uint32_t pwm_Hz;      /**< PWM Frequency in Hertz */
        uint32_t deadtime_nS; /**< Deadtime in Nanoseconds */
        float duty_max;       /**< Maximum allowable duty cycle */
        float duty_min;       /**< Minimum allowable duty cycle */
    } fd_mc2p_config_t;

    /**
     * @brief Main handle structure for the MC2P driver instance.
     */
    typedef struct
    {
        fd_mc2p_config_t config;
        int16_t adc_to_uV;
        float adc_ref_V;
        float internal_ref_V;
        volatile fd_mc2p_mode_t mode;
        volatile fd_mc2p_sector_t sector_last;
        uint32_t timer_max_q15;
        uint16_t duty_max_q15;
        uint16_t duty_min_q15;
        int16_t dutyu_q15;
        int16_t dutyv_q15;
        int32_t sync_scale_q31[MC2P_SYNC_CHANNELS][2];
        uint8_t sync_rank_scale[4];
        bool offset_calibration;
        volatile uint8_t offset_calibration_remaining_samples;
        volatile uint16_t offset_calibration_sum[4];
        uint16_t dtc_comp_q15;
        uint8_t dtc_state;
    } fd_mc2p_handle_t;

    /** @brief Data structure for SVM-specific measurements. */
    typedef struct
    {
        int32_t vbus_q31;
        int32_t cbus_q31;
    } fd_mc2p_data_t;

/* Conversion Macros */
#define FD_MC2P_VS_TO_FLOAT(vs) ((float)(((int64_t)(vs) * FD_MC2P_VS_SCALE) >> 31))
#define FD_MC2P_CS_TO_FLOAT(cs) ((float)(((int64_t)(cs) * FD_MC2P_CS_SCALE) >> 31))
#define FD_MC2P_FLOAT_TO_VS(f) ((int32_t)(((float)(f) / FD_MC2P_VS_SCALE) * INT32_MAX))
#define FD_MC2P_FLOAT_TO_CS(f) ((int32_t)(((float)(f) / FD_MC2P_CS_SCALE) * INT32_MAX))
#define FD_MC2P_SYNC_CHANNELS_NUM()(MC2P_SYNC_CHANNELS)

    // TODO  FINISH ADC IMPLEMENTATION FOR 1)TRAP & 2)SVM
    void mc2p_irq_bind(fd_mc2p_handle_t *h);

    void fd_mc2p_init(fd_mc2p_handle_t *h);
    void fd_mc2p_reconfigure_pwm(fd_mc2p_handle_t *h);

    void fd_mc2p_set_gain(fd_mc2p_handle_t *h, fd_mc2p_sync_t s, float gain);

    void fd_mc2p_set_sync_scale(fd_mc2p_handle_t *h, const float scales[MC2P_SYNC_CHANNELS][2]);

    void fd_mc2p_bg_startConv(fd_mc2p_handle_t *h);

    uint8_t fd_mc2p_bg_channels(fd_mc2p_handle_t *h);

    uint8_t fd_mc2p_read_bg(fd_mc2p_handle_t *h, float *scanData);

    uint8_t fd_mc2p_bg_isReady(fd_mc2p_handle_t *h);

    void fd_mc2p_read_sync(fd_mc2p_handle_t *h, void *scanData);

    float fd_mc2p_adc_read_single(fd_mc2p_handle_t *h, uint32_t channel);

    void fd_mc2p_write_phase_state(fd_mc2p_handle_t *h, fd_mc2p_phase_state_t state_u, fd_mc2p_phase_state_t state_v, fd_mc2p_phase_state_t state_w);


    void fd_mc2p_write_phase_duty(fd_mc2p_handle_t *h, int16_t duty_u_q15, int16_t duty_v_q15);


    void fd_mc2p_write_float(fd_mc2p_handle_t *h);


    __attribute__((weak)) void fd_mc2p_tickerCallback(void);

    __attribute__((weak)) void fd_mc2p_sync_postScanCallback(void);
#ifdef __cplusplus
}
#endif

#endif
