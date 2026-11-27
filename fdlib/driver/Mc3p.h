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
#define FDMC3P_SYNC_CHANNELS_MAX 7
#define FDMC3P_BG_CHANNELS_MAX 8
#define MC3P_SATURATE(v, min, max) (((v) > (max)) ? (max) : (((v) < (min)) ? (min) : (v)))
static inline int16_t FdMc3p_SaturateQ15(int32_t value)
{
    if (value > INT16_MAX) return INT16_MAX;
    if (value < INT16_MIN) return INT16_MIN;
    return (int16_t)value;
}
#define Q15_HALF        16384   // 0.5 × 32768
#define Q15_SQRT3_BY_2  28378   // 0.8660254 × 32768


#include <stdbool.h>
#include "Drv.h"
#include <math.h>

    typedef enum
    {
        FdMc3pCsType_None               = 0,
        FdMc3pCsType_TripleShunt        = 1,
        FdMc3pCsType_DoubleShunt        = 2,
        FdMc3pCsType_SingleShunt        = 3,
        FdMc3pCsType_Inline             = 4
    } FdMc3pCsType;
    
    typedef enum
    {
        FdMc3pSector_Float = 0, /**< Floating state (all MOSFETs OFF) */
        FdMc3pSector_Trap1,     /**< Trapezoidal Sector 1 */
        FdMc3pSector_Trap2,     /**< Trapezoidal Sector 2 */
        FdMc3pSector_Trap3,     /**< Trapezoidal Sector 3 */
        FdMc3pSector_Trap4,     /**< Trapezoidal Sector 4 */
        FdMc3pSector_Trap5,     /**< Trapezoidal Sector 5 */
        FdMc3pSector_Trap6,     /**< Trapezoidal Sector 6 */
        FdMc3pSector_Svm1,      /**< SVM Sector 1 */
        FdMc3pSector_Svm2,      /**< SVM Sector 2 */
        FdMc3pSector_Svm3,      /**< SVM Sector 3 */
        FdMc3pSector_Svm4,      /**< SVM Sector 4 */
        FdMc3pSector_Svm5,      /**< SVM Sector 5 */
        FdMc3pSector_Svm6       /**< SVM Sector 6 */
    } FdMc3pSector;

    /**
     * @brief Enumeration for motor control modulation modes.
     */
    typedef enum
    {
        FdMc3pMode_None = 0,
        FdMc3pMode_Trap,
        FdMc3pMode_Svm,
        FdMc3pMode_Calib
    } FdMc3pMode;

    
/** @brief Check if sector is within Space Vector Modulation range. */
#define IS_SVM_SECTOR(sector) (sector >= FdMc3pSector_Svm1 && sector <= FdMc3pSector_Svm6)
/** @brief Check if sector is within Trapezoidal range. */
#define IS_TRAP_SECTOR(sector) (sector >= FdMc3pSector_Trap1 && sector <= FdMc3pSector_Trap6)

    /**
     * @brief Enumeration for the state of each motor phase.
     */
    typedef enum
    {
        FdMc3pPhaseState_Float = 0, /**< Phase is floating */
        FdMc3pPhaseState_HPwm,     /**< High-side PWM active */
        FdMc3pPhaseState_LPwm,     /**< Low-side PWM active */
        FdMc3pPhaseState_Comp,      /**< Complementary PWM active */
        FdMc3pPhaseState_HOn,      /**< High-side strictly ON */
        FdMc3pPhaseState_LOn       /**< Low-side strictly ON */
    } FdMc3pPhaseState;

    typedef struct {
        FdMc3pPhaseState phase_state[3]; // A,B,C: COMP, L_ON, FLOAT
    } FdMc3pSectorMap;
    
    const FdMc3pSectorMap trap_table[6] = {
    {{ mc3p.trap_table }}
    };

    /**
     * @brief Mapping for synchronized ADC measurements.
     */
    typedef enum
    {
        FdMc3pSync_VsBus = 0,           /**< DC Bus Voltage */
        FdMc3pSync_VsU,                 /**< Phase U Voltage */
        FdMc3pSync_VsV,                 /**< Phase V Voltage */
        FdMc3pSync_VsW,                 /**< Phase W Voltage */
        FdMc3pSync_CsU,                 /**< Phase U Current */
        FdMc3pSync_CsV,                 /**< Phase V Current */
        FdMc3pSync_CsW,                 /**< Phase W Current */
        FdMc3pSync_CsBus = FdMc3pSync_CsU, /**< DC Bus Current */
    } FdMc3pSync;

    /**
     * @brief Configuration structure for MC3P PWM and duty limits.
     */
    typedef struct
    {
        uint32_t pwm_hz;      /**< PWM Frequency in Hertz */
        uint32_t deadtime_ns; /**< Deadtime in Nanoseconds */
        float maxDuty;       /**< Maximum allowable duty cycle */
        float minDuty;       /**< Minimum allowable duty cycle */
        float dtcThresh;
        bool dtcActive;
        bool offsetCalibration;
    } FdMc3pConfig;

    /**
     * @brief Main handle structure for the MC3P driver instance.
     */
    typedef struct
    {
        FdMc3pConfig config;
        volatile FdMc3pMode mode;
        volatile FdMc3pSector sectorLast;
        uint32_t maxTimer;
        uint16_t maxDuty;
        uint16_t minDuty;
        int16_t dutyU;
        int16_t dutyV;
        int16_t dutyW;
        int32_t syncScale[FDMC3P_SYNC_CHANNELS_MAX][2];
        uint16_t bgData[FDMC3P_BG_CHANNELS_MAX];
        uint8_t bgChannels;
        int32_t rankScale[4][2];
        uint16_t dtcComp_vsQ15;
        int32_t dtcThresh_csQ31;
        uint8_t dtcState;
    } FdMc3pHandle;

    /** @brief Data structure for SVM-specific measurements. */
    typedef struct
    {
        int32_t vBus_vsQ31;
        int32_t cU_csQ31;
        int32_t cV_csQ31;
        int32_t cW_csQ31;
    } FdMc3pSvmData;

    /** @brief Data structure for Trapezoidal-specific measurements. */
    typedef struct
    {
        int32_t vBus_vsQ31;
        int32_t vBemf_vsQ31;
        int32_t cBus_csQ31;
    } FdMc3pTrapData;

/* Conversion Macros */
#define FDMC3P_VS_SCALE {{mc3p.vs_scale}}
#define FDMC3P_CS_SCALE {{mc3p.cs_scale}}
#define FDMC3P_VS2F(vs) ((float)((int64_t)(vs) * FDMC3P_VS_SCALE) / INT32_MAX)
#define FDMC3P_CS2F(cs) ((float)((int64_t)(cs) * FDMC3P_CS_SCALE) / INT32_MAX)
#define FDMC3P_F2VS(f) ((int32_t)(((float)(f) / FDMC3P_VS_SCALE) * INT32_MAX))
#define FDMC3P_F2CS(f) ((int32_t)(((float)(f) / FDMC3P_CS_SCALE) * INT32_MAX))
static inline void PrvMc3p_DtcUpdatePolarity(FdMc3pHandle *h, int32_t current, uint8_t bit)
{
    if (current > h->dtcThresh_csQ31) h->dtcState |= bit;
    else if (current < -h->dtcThresh_csQ31) h->dtcState &= (uint8_t)~bit;
}
static inline int16_t PrvMc3p_DtcComp(FdMc3pHandle *h, uint8_t bit){return (h->dtcState & bit)? h->dtcComp_vsQ15 : -h->dtcComp_vsQ15;}
static inline void PrvMc3p_DtcUpdateComp(FdMc3pHandle *h){h->dtcComp_vsQ15 = h->config.dtcActive?(int16_t)(((float)h->config.deadtime_ns*h->config.pwm_hz/1e9)*INT16_MAX + 0.5) : 0;}
{% for inst in mc3p.instances %}
void PrvMc3p_{{inst.name}}_SectorUpdate(FdMc3pHandle *h, FdMc3pSector sector);
void PrvMc3p_{{inst.name}}_HwInit(FdMc3pHandle *h);
void PrvMc3p_{{inst.name}}_OffsetCalibration(FdMc3pHandle *h);
static inline FdMc3pMode FdMc3p_Sector2Mode(FdMc3pSector sector)
{
    if (sector >= FdMc3pSector_Trap1 &&
        sector <= FdMc3pSector_Trap6)
        return FdMc3pMode_Trap;

    if (sector >= FdMc3pSector_Svm1 &&
        sector <= FdMc3pSector_Svm6)
        return FdMc3pMode_Svm;

    return FdMc3pMode_None;
}
static inline void FdMc3p_{{inst.name}}_Init(FdMc3pHandle *h)
{
    h->maxDuty = (uint16_t)((h->config.maxDuty) * 0x7FFF);
    h->minDuty = (uint16_t)((h->config.minDuty) * 0x7FFF);
    h->dtcThresh_csQ31 = FDMC3P_F2CS(h->config.dtcThresh);
    //compute deadtme compensation
    PrvMc3p_DtcUpdateComp(h);
    
    h->sectorLast = FdMc3pSector_Float;
    h->mode        = FdMc3pMode_None;
    PrvMc3p_{{inst.name}}_HwInit(h);
    if(h->config.offsetCalibration)PrvMc3p_{{inst.name}}_OffsetCalibration(h);
}
void FdMc3p_{{inst.name}}_ReconfigurePwm(FdMc3pHandle *h);
void FdMc3p_{{inst.name}}_BgStartConv(FdMc3pHandle *h);
void FdMc3p_{{inst.name}}_ReadSync(FdMc3pHandle *h, void *scanData);
void FdMc3p_{{inst.name}}_WritePhaseState(FdMc3pHandle *h, FdMc3pPhaseState stateU, FdMc3pPhaseState stateV, FdMc3pPhaseState stateW);
void FdMc3p_{{inst.name}}_WritePhaseDuty(FdMc3pHandle *h, int16_t dutyU, int16_t dutyV, int16_t dutyW);
static inline void FdMc3p_{{inst.name}}_WriteFloat(FdMc3pHandle *h)
{    
    FdMc3p_{{inst.name}}_WritePhaseState(h, FdMc3pPhaseState_Float, FdMc3pPhaseState_Float, FdMc3pPhaseState_Float);
    FdMc3p_{{inst.name}}_WritePhaseDuty(h, 0, 0, 0);
    PrvMc3p_{{inst.name}}_SectorUpdate(h, FdMc3pSector_Float);
    h->mode = FdMc3pMode_None;
}
static inline void FdMc3p_{{inst.name}}_WriteTrap(FdMc3pHandle *h, FdMc3pSector sector, uint16_t duty)
{
    if(sector < FdMc3pSector_Trap1 || sector > FdMc3pSector_Trap6)
        return;

    // Sector update
    if(h->sectorLast != sector)
    {
        PrvMc3p_{{inst.name}}_SectorUpdate(h, sector);
    }
    h->dutyU = FdMc3p_SaturateQ15((int32_t)duty + PrvMc3p_DtcComp(h, 1<<3));
    h->dutyV = h->dutyU;
    h->dutyW = h->dutyU;
    FdMc3p_{{inst.name}}_WritePhaseDuty(h, h->dutyU, h->dutyV, h->dutyW);
}
static inline void FdMc3p_{{inst.name}}_WriteSvm(FdMc3pHandle *h, int16_t alpha, int16_t beta) 
{
    int32_t vmax, vmin, voff, vcm;
    int32_t dutyU, dutyV, dutyW;
    /* αβ → phase (normalized Q15) */
    dutyU = alpha;
    dutyV = (-(int32_t)Q15_HALF * alpha + (int32_t)Q15_SQRT3_BY_2 * beta) >> 15;
    dutyW = (-(int32_t)Q15_HALF * alpha - (int32_t)Q15_SQRT3_BY_2 * beta) >> 15;
    /* SVPWM zero-sequence injection */
    uint8_t b0 = (dutyU >= 0);
    uint8_t b1 = (dutyV >= 0);
    uint8_t b2 = (dutyW >= 0);
    // 3-bit code
    uint8_t code = (b2 << 2) | (b1 << 1) | b0;
    FdMc3pSector sector;
    switch(code)
    {
        case 0b001: sector = FdMc3pSector_Svm1; break;
        case 0b011: sector = FdMc3pSector_Svm2; break;
        case 0b010: sector = FdMc3pSector_Svm3; break;
        case 0b110: sector = FdMc3pSector_Svm4; break;
        case 0b100: sector = FdMc3pSector_Svm5; break;
        case 0b101: sector = FdMc3pSector_Svm6; break;
        default: sector = FdMc3pSector_Float; break; // should not happen
    }
    // Sector update
    if(h->sectorLast != sector)
    {
        PrvMc3p_{{inst.name}}_SectorUpdate(h, sector);
    }
    dutyU += PrvMc3p_DtcComp(h, 1<<0);
    dutyV += PrvMc3p_DtcComp(h, 1<<1);
    dutyW += PrvMc3p_DtcComp(h, 1<<2);

    /* SVPWM zero-sequence injection */
    vmax = dutyU;
    if (dutyV > vmax) vmax = dutyV;
    if (dutyW > vmax) vmax = dutyW;

    vmin = dutyU;
    if (dutyV < vmin) vmin = dutyV;
    if (dutyW < vmin) vmin = dutyW;

    voff = (vmax + vmin) >> 1;
    vcm = (h->maxDuty + h->minDuty) >> 1;
    dutyU = (dutyU - voff) + vcm;
    dutyV = (dutyV - voff) + vcm;
    dutyW = (dutyW - voff) + vcm;
    
    h->dutyU = FdMc3p_SaturateQ15(dutyU);
    h->dutyV = FdMc3p_SaturateQ15(dutyV);
    h->dutyW = FdMc3p_SaturateQ15(dutyW);
    FdMc3p_{{inst.name}}_WritePhaseDuty(h, h->dutyU, h->dutyV, h->dutyW);
    h->sectorLast = sector;
}
__attribute__((weak)) void FdMc3p_{{inst.name}}_SyncPostScanCallback(void);
__attribute__((weak)) void FdMc3p_{{inst.name}}_BgPostScanCallback(void);
extern FdMc3pHandle {{inst.name}};
{% endfor %}

// ==========================================
// UNIFIED COMPILE-TIME DISPATCH MACROS
// ==========================================

#define FDMC3P_DISPATCH_IMPL(inst, suffix) FdMc3p_##inst##suffix
#define FDMC3P_DISPATCH(inst, suffix) FDMC3P_DISPATCH_IMPL(inst, suffix)

#define FdMc3p_Init(inst) FDMC3P_DISPATCH(inst, _Init)(&(inst))
#define FdMc3p_ReconfigurePwm(inst) FDMC3P_DISPATCH(inst, _ReconfigurePwm)(&(inst))
#define FdMc3p_BgStartConv(inst) FDMC3P_DISPATCH(inst, _BgStartConv)(&(inst))
#define FdMc3p_ReadSync(inst, data) FDMC3P_DISPATCH(inst, _ReadSync)(&(inst), data)
#define FdMc3p_WritePhaseState(inst, state_u, state_v, state_w) FDMC3P_DISPATCH(inst, _WritePhaseState)(&(inst), state_u, state_v, state_w)
#define FdMc3p_WritePhaseDuty(inst, duty_u, duty_v, duty_w) FDMC3P_DISPATCH(inst, _WritePhaseDuty)(&(inst), duty_u, duty_v, duty_w)
#define FdMc3p_WriteFloat(inst) FDMC3P_DISPATCH(inst, _WriteFloat)(&(inst))
#define FdMc3p_WriteTrap(inst, sector, duty) FDMC3P_DISPATCH(inst, _WriteTrap)(&(inst), sector, duty)
#define FdMc3p_WriteSvm(inst, a, b) FDMC3P_DISPATCH(inst, _WriteSvm)(&(inst), a, b)



#ifdef __cplusplus
}
#endif

#endif
