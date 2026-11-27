
#include "Mc3p.h"
#include "Adc.h"
#include <string.h>
#include "Drv.h"
#include "System.h"
#include "stm32{{device.family}}xx_ll_adc.h"
#include "stm32{{device.family}}xx_ll_tim.h"
#include "stm32{{device.family}}xx_ll_bus.h"
#include "stm32{{device.family}}xx_ll_gpio.h"
#include "stm32{{device.family}}xx_ll_dma.h"
#include "stm32{{device.family}}xx.h"

//============================================================
//TODO:Single Shunt current sensing implementation
/// Instance templated code with jinja2 codegen
//TODO: add break feature, BKIN as a core feature

#define CC1E_MASK       (TIM_CCER_CC1E)
#define CC2E_MASK       (TIM_CCER_CC2E)
#define CC3E_MASK       (TIM_CCER_CC3E)
#define CC1NE_MASK      (TIM_CCER_CC1NE)
#define CC2NE_MASK      (TIM_CCER_CC2NE)
#define CC3NE_MASK      (TIM_CCER_CC3NE)

#define OC1M_MASK       (0b111 << TIM_CCMR1_OC1M_Pos)
#define OC1M_PWM        (0b110 << TIM_CCMR1_OC1M_Pos)
#define OC1M_ACTIVE     (0b101 << TIM_CCMR1_OC1M_Pos)
#define OC1M_INACTIVE   (0b100 << TIM_CCMR1_OC1M_Pos)

#define OC2M_MASK       (0b111 << TIM_CCMR1_OC2M_Pos)
#define OC2M_PWM        (0b110 << TIM_CCMR1_OC2M_Pos)
#define OC2M_ACTIVE     (0b101 << TIM_CCMR1_OC2M_Pos)
#define OC2M_INACTIVE   (0b100 << TIM_CCMR1_OC2M_Pos)

#define OC3M_MASK       (0b111 << TIM_CCMR2_OC3M_Pos)
#define OC3M_PWM        (0b110 << TIM_CCMR2_OC3M_Pos)
#define OC3M_ACTIVE     (0b101 << TIM_CCMR2_OC3M_Pos)
#define OC3M_INACTIVE   (0b100 << TIM_CCMR2_OC3M_Pos)

typedef struct {
    uint32_t channel;
    uint8_t  scale_idx;
    int8_t sign;
} inj_rank_t;


#define COPY_SCALE_SIGNED(dst, src, sign)do { \
    h->rankScale[dst][0] = (sign == -1)? -h->syncScale[src][0] : h->syncScale[src][0]; \
    h->rankScale[dst][1] = h->syncScale[src][1]; \
} while (0)
#define COPY_SCALE(dst, src)  COPY_SCALE_SIGNED(dst, src, 1)

{% for inst in mc3p.instances %}
{% if inst.analog.bg | length > 0 %}
{% set bg_dma = dma_parse(inst.dma["bg"], device.dma_arch) %}
{% set use_bg = true %}
{% set bg_length = inst.analog.bg | length %}
{% endif %}



static inj_rank_t {{inst.name}}_trapVsTable[6] = 
{
    {% for entry in inst.trapVs_vector %}
    { LL_ADC_CHANNEL_{{ entry.ch }}, FdMc3pSync_Vs{{ entry.phase | upper }}, {{entry.sign}}  },
    {% endfor %}
};
static inj_rank_t {{inst.name}}_trapCsTable[6] = 
{
    {% for entry in inst.trapCs_vector %}
    { LL_ADC_CHANNEL_{{ entry.ch }}, FdMc3pSync_Cs{{ entry.phase | upper }}, {{entry.sign}} }, 
    {% endfor %}
};

static void {{inst.name}}_TimInit(FdMc3pHandle *h){
    //ENABLE Peripheral CLOCK
    __FDGEN__CLK_EN(TIM{{inst.pwm.tim}})
    TIM_TypeDef * const tim = TIM{{inst.pwm.tim}};
    //===================================================== 
    // BASIC TIMER CONFIGURATION
    //=====================================================
    LL_TIM_InitTypeDef timInit =
    {
        .Prescaler = 0,
        .CounterMode = LL_TIM_COUNTERMODE_CENTER_UP,
        .Autoreload = __LL_TIM_CALC_ARR(PeriphBusClk(tim), 1, h->config.pwm_hz),
        .ClockDivision = LL_TIM_CLOCKDIVISION_DIV1,
        .RepetitionCounter = 0
    };
    LL_TIM_EnableARRPreload(tim);
    LL_TIM_SetUpdateSource(tim, LL_TIM_UPDATESOURCE_REGULAR);
    LL_TIM_Init(tim, &timInit);
    //===================================================== 
    // ADVANCED TIMER CONFIGURATION (BREAK AND DEADTIME)
    //=====================================================
    LL_TIM_BDTR_InitTypeDef timBDTRInit = {
        .OSSRState = LL_TIM_OSSR_ENABLE,
        .OSSIState = LL_TIM_OSSI_DISABLE,
        .LockLevel = LL_TIM_LOCKLEVEL_OFF,
        .DeadTime = (uint8_t)(__LL_TIM_CALC_DEADTIME(PeriphBusClk(tim), LL_TIM_GetClockDivision(tim), h->config.deadtime_ns)),
        .BreakState = LL_TIM_BREAK_DISABLE,
        .BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH,
        .AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_DISABLE
    };
    LL_TIM_BDTR_Init(tim, &timBDTRInit);
    //===================================================== 
    // IDENTICAL OUTPUT CHANNEL CONFIGURATION ON CHANNEL 1,2,3
    //=====================================================
    {% for ch in ["u", "v", "w"]%}
    LL_TIM_OC_InitTypeDef {{ch}}_tim_OCInit = 
    {
        .OCMode = LL_TIM_OCMODE_INACTIVE,
        .OCState = LL_TIM_OCSTATE_ENABLE,
        .OCNState = LL_TIM_OCSTATE_DISABLE,
        .CompareValue = 0,
        .OCPolarity = {{ inst.pwm[ch + "h"][0] }} ? LL_TIM_OCPOLARITY_HIGH : LL_TIM_OCPOLARITY_LOW,
        .OCNPolarity = {{ inst.pwm[ch + "l"][0] }} ? LL_TIM_OCPOLARITY_HIGH : LL_TIM_OCPOLARITY_LOW,
        .OCIdleState = {{ inst.pwm[ch + "h"][0] }} ? LL_TIM_OCIDLESTATE_LOW : LL_TIM_OCIDLESTATE_HIGH,
        .OCNIdleState = {{ inst.pwm[ch + "l"][0] }} ? LL_TIM_OCIDLESTATE_LOW : LL_TIM_OCIDLESTATE_HIGH
    };
    LL_TIM_OC_Init(tim, LL_TIM_CHANNEL_CH{{inst.pwm[ch + "h"][1]}}, &{{ch}}_tim_OCInit);
    LL_TIM_OC_EnablePreload(tim, LL_TIM_CHANNEL_CH{{inst.pwm[ch + "h"][1]}});
    {% endfor %}
    LL_TIM_CC_EnablePreload(tim);

    //## Configure use TIM1 CH4 for adc triggering (we exlusively use CH4 for adc triggering)
    //==================================================================
    // USE CHANNEL 4 Exlusively FOR Injected Adc channel (adcPWM Scan)
    //==================================================================
    LL_TIM_OC_InitTypeDef ch4_tim_OCInit = 
    {
        .OCMode = LL_TIM_OCMODE_PWM1,           // Change to PWM mode
        .OCState = LL_TIM_OCSTATE_ENABLE,       // Enable the output
        .CompareValue = (LL_TIM_GetAutoReload(tim) + 1) / 2, // Mid-point for center-aligned
        .OCPolarity = LL_TIM_OCPOLARITY_HIGH,   // Rising edge at compare match
        .OCIdleState = LL_TIM_OCIDLESTATE_LOW,
    };
    LL_TIM_OC_EnablePreload(tim, LL_TIM_CHANNEL_CH4);
    LL_TIM_OC_Init(tim, LL_TIM_CHANNEL_CH4, &ch4_tim_OCInit);
    LL_TIM_SetTriggerOutput(tim, LL_TIM_TRGO_OC4REF);
   
    //GET TIMER MAX MAX VALUE TO BE USED FOR DUTY CALCULATION
    h->maxTimer = LL_TIM_GetAutoReload(tim);
    // Finally enable the timer (Configuration done)
    LL_TIM_EnableAllOutputs(tim);
    LL_TIM_EnableCounter(tim);
}
void {{inst.name}}_DmaIntInit(FdMc3pHandle *h)
{
    {% if use_bg %}
    // Enable DMA clock
    __FDGEN__CLK_EN({{bg_dma.dma}});
    DMA_TypeDef *bg_dma = {{bg_dma.dma}};
    uint32_t bg_stream = {{bg_dma.stream}};
    uint32_t bg_channel = {{bg_dma.channel}};
     uint32_t bg_prio = {{bg_dma.prio}};
    // =================================================================
    // DMA for Regular Scan (Slow Monitoring Channels)
    // =================================================================
    LL_DMA_SetChannelSelection(bg_dma, bg_stream, bg_channel);
    // Configure DMA parameters for ADC regular conversions
    LL_DMA_SetDataTransferDirection(bg_dma, bg_stream, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetStreamPriorityLevel(bg_dma, bg_stream, bg_prio);
    LL_DMA_SetMode(bg_dma, bg_stream, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(bg_dma, bg_stream, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(bg_dma, bg_stream, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(bg_dma, bg_stream, LL_DMA_PDATAALIGN_HALFWORD);
    LL_DMA_SetMemorySize(bg_dma, bg_stream, LL_DMA_MDATAALIGN_HALFWORD);
    LL_DMA_DisableFifoMode(bg_dma, bg_stream);
    // Set peripheral address (ADC data register)
    LL_DMA_SetPeriphAddress(bg_dma, bg_stream, (uint32_t)&(ADC{{inst.analog.adc}}->DR));
    // Set memory address (double buffer start)
    LL_DMA_SetMemoryAddress(bg_dma, bg_stream, (uint32_t)h->bgData[0]);
    // Set data length (total samples across both buffers)
    LL_DMA_SetDataLength(bg_dma, bg_stream, {{bg_length}});
    // Enable DMA transfer complete and half transfer interrupts
    LL_DMA_EnableIT_TC(bg_dma, bg_stream);
    // Enable DMA stream
    LL_DMA_EnableStream(bg_dma, bg_stream);
    // Enable JEOS (Injected End Of Sequence) interrupt in ADC
    LL_ADC_EnableIT_JEOS(ADC{{inst.analog.adc}});
    __FDGEN__IRQ_EN({{bg_dma.irq_name}}, {{inst.irq["bg"]}});
    __FDGEN__IRQ_BIND({{bg_dma.irq_name}}, {{inst.name}}_bg_dma_irqhandler);
    {% endif %}
    __FDGEN__IRQ_EN(TIM{{inst.pwm.tim}}_UP, {{inst.irq["tim"]}});
    __FDGEN__IRQ_EN(ADC, {{inst.irq["sync"]}});
    __FDGEN__IRQ_BIND(TIM{{inst.pwm.tim}}_UP, {{inst.name}}_tim_up_irqhandler);
    __FDGEN__IRQ_BIND(ADC,{{inst.name}}_adc_irqhandler);
}
void {{inst.name}}_AdcInit(FdMc3pHandle *h)
{
    __FDGEN__CLK_EN(ADC{{ inst.analog.adc }})
    ADC_TypeDef *adc = ADC{{ inst.analog.adc }};
    if (!FdAdc_CommonInitialized)
    {
        LL_ADC_CommonInitTypeDef adcCommonInitStruct = {
            .CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV2
            #ifdef LL_ADC_MULTI_INDEPENDENT
            .Multimode = LL_ADC_MULTI_INDEPENDENT,
            .MultiDMATransfer = LL_ADC_MULTI_REG_DMA_EACH_ADC,
            .MultiTwoSamplingDelay = LL_ADC_MULTI_TWOSAMPLINGDELAY_5CYCLES
            #endif
        };
        LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(adc), &adcCommonInitStruct);
    }
    LL_ADC_InitTypeDef adcInitStruct = {
        .Resolution = LL_ADC_RESOLUTION_12B,
        .DataAlignment = LL_ADC_DATA_ALIGN_RIGHT,
        .SequencersScanMode = LL_ADC_SEQ_SCAN_ENABLE
    };
    
    LL_ADC_Init(adc, &adcInitStruct);

    // Enable ADC
    if (LL_ADC_IsEnabled(adc) == 0) 
    {
        LL_ADC_Enable(adc);
    }

    //Calibrate Adc
    FdAdc_Calibrate();
    
    //bg sequencer and rank config
    {% if use_bg %}

    /* Configure regular ADC sequence */
    LL_ADC_REG_InitTypeDef adcRegInitStruct = {
        .TriggerSource = LL_ADC_REG_TRIG_SOFTWARE,
        .SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_{{ inst.analog.bg | length }}RANKS,
        .SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE,
        .ContinuousMode = LL_ADC_REG_CONV_CONTINUOUS,
        .DMATransfer = LL_ADC_REG_DMA_TRANSFER_UNLIMITED
    };

    LL_ADC_REG_Init(adc, &adcRegInitStruct);

    /* Configure channel ranks and sampling times */
    {% for signal, config in inst.analog.bg.items() %} 
    LL_ADC_REG_SetSequencerRanks(adc, LL_ADC_REG_RANK_{{ loop.index }}, LL_ADC_CHANNEL_{{ config[0] }});
    LL_ADC_SetChannelSamplingTime(adc, LL_ADC_CHANNEL_{{ config[0] }}, LL_ADC_SAMPLINGTIME_{{ config[1] | default("56") }}CYCLES );
    {% endfor %}

    {% endif %}

    // Initialize ADC injected group for time-critical measurements
    LL_ADC_INJ_InitTypeDef adcInjInitStruct = {
        .TriggerSource = LL_ADC_INJ_TRIG_EXT_TIM1_TRGO,
        .SequencerLength = LL_ADC_INJ_SEQ_SCAN_ENABLE_4RANKS,
        .SequencerDiscont = LL_ADC_INJ_SEQ_DISCONT_DISABLE,
        .TrigAuto = LL_ADC_INJ_TRIG_INDEPENDENT
    };                                                      

    // Set sampling times for all PWM Scan channels (injected group)
    {% for signal, config in inst.analog.inj.items()  %}
    LL_ADC_SetChannelSamplingTime(adc, LL_ADC_CHANNEL_{{ config[0] }},  LL_ADC_SAMPLINGTIME_{{ config[1] | default("56") }}CYCLES );
    {% endfor %}

    LL_ADC_INJ_Init(adc, &adcInjInitStruct);
    //ADC CONFIGURATIONS AND LINKAGE TO TIM1TRGO
    LL_ADC_INJ_StartConversionExtTrig(adc, LL_ADC_INJ_TRIG_EXT_RISING);
    // Enable ADC interrupts
    LL_ADC_EnableIT_JEOS(adc);  // Injected sequence end interrupt  
}
void PrvMc3p_{{inst.name}}_HwInit(FdMc3pHandle *h)
{
    {{inst.name}}_TimInit(h);
    {{inst.name}}_DmaIntInit(h);
    {{inst.name}}_AdcInit(h);
}

//Mode and sector changes
void {{inst.name}}_SvmMode(FdMc3pHandle *h)
{
    //do needed pwm and adc updates
    ADC_TypeDef *adc = ADC{{ inst.analog.adc }};
    FdMc3p_{{inst.name}}_WritePhaseState(h, FdMc3pPhaseState_Comp, FdMc3pPhaseState_Comp, FdMc3pPhaseState_Comp);
    LL_ADC_INJ_SetSequencerLength(adc, LL_ADC_INJ_SEQ_SCAN_ENABLE_4RANKS);
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_1, {{inst.analog.inj["vsbus"][0]}});
    COPY_SCALE(0, FdMc3pSync_VsBus);
    //enable all current sensors
    {% for phase, ch in inst.analog.inj.items() if phase.startswith("cs")%} 
        LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_{{ loop.index + 1 }}, LL_ADC_CHANNEL_{{ ch[0] }});
        COPY_SCALE({{ loop.index }}, FdMc3pSync_Cs{{ phase[2:] | upper }});
    {% endfor %}
}
void {{inst.name}}_TrapMode(FdMc3pHandle *h)
{
    //do needed pwm and adc updates
    ADC_TypeDef *adc = ADC{{ inst.analog.adc }};
    LL_ADC_INJ_SetSequencerLength(adc, LL_ADC_INJ_SEQ_SCAN_ENABLE_4RANKS);
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_1, {{inst.analog.inj["vsbus"][0]}});
    COPY_SCALE(0, FdMc3pSync_VsBus);
    {% if mc3p.current_sensing.topology == "single_shunt" %}
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_3, {{inst.analog.inj["csbus"][0]}});
    COPY_SCALE_SIGNED(2, FdMc3pSync_CsBus, -1);
    {% endif %}
}
void {{inst.name}}_TrapSector(FdMc3pHandle *h, FdMc3pSector sector)
{
    //do needed pwm and adc updates
    ADC_TypeDef *adc = ADC{{ inst.analog.adc }};
    const inj_rank_t *trapCs_map = &{{inst.name}}_trapCsTable[sector - 1];
    const inj_rank_t *trapVs_map = &{{inst.name}}_trapVsTable[sector - 1];
    const FdMc3pSectorMap *pwm_map = &trap_table[sector - 1];
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_3, trapCs_map->channel);
    COPY_SCALE(1, trapVs_map->scale_idx);
    {% if not mc3p.current_sensing.topology == "single_shunt" %}
    FdMc3p_{{inst.name}}_WritePhaseState(h, pwm_map->phase_state[0], pwm_map->phase_state[1], pwm_map->phase_state[2]);
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_3, trapCs_map->channel);
    COPY_SCALE_SIGNED(2, trapCs_map->scale_idx, trapCs_map->sign);
    {% endif %}
}
void {{inst.name}}_SvmSector(FdMc3pHandle *h, FdMc3pSector sector)
{
    TIM_TypeDef * const tim = TIM{{inst.pwm.tim}};
    ADC_TypeDef *adc = ADC{{ inst.analog.adc }};
    {% if mc3p.current_sensing.topology == "three_shunt" %}
    {% elif mc3p.current_sensing.topology == "two_shunt" %}
    {% elif mc3p.current_sensing.topology == "two_inline" %}
    {% elif mc3p.current_sensing.topology == "single_shunt" %}
    {% endif %}
}

void PrvMc3p_{{inst.name}}_SectorUpdate(FdMc3pHandle *h, FdMc3pSector sector)
{//This method is called on sector changes, mode changes are inferred from sector
    //Check for mode changes....
    FdMc3pMode cm = FdMc3p_Sector2Mode(sector);
    if(cm != h->mode)//mode changed... call mode update functions
    {
        switch (cm)
        {
        case FdMc3pMode_Trap:
            {{inst.name}}_TrapMode(h);
            break;
        case FdMc3pMode_Svm:
            {{inst.name}}_SvmMode(h);
            break;
        default:
            break;
        }
        h->mode = cm;
    }
    //update sector
    switch (h->mode)
    {
    case FdMc3pMode_Trap:
        {{inst.name}}_TrapSector(h, sector);
        break;
    case FdMc3pMode_Svm:
        {{inst.name}}_SvmSector(h, sector);
        break;
    default:
        break;
    }

    h->sectorLast = sector;
}
void PrvMc3p_{{inst.name}}_OffsetCalibration(FdMc3pHandle *h)
{
    // Set all phases to Low (0,0,0)
    // Set mode to calibration
    FdMc3p_{{inst.name}}_WriteFloat(h);
    h->mode = FdMc3pMode_Calib;
    const uint32_t calibration_samples_log2 = {{mc3p["current_sensing"].calibration_samples_log2 | default("10")}};
    const uint32_t sample_count = 1U << calibration_samples_log2;
    uint32_t calibration_sum[3] = {0U, 0U, 0U};
    //Lets start by doing offset calibration for current sensors
    //Hardware Specific code
    //Disable the JEOS IRQ temporarily and timer triggering, we will be doing SW triggering for now, this is a blocking method
    // Save the ADC configuration that this routine will modify.
    ADC_TypeDef *adc = ADC{{ inst.analog.adc }};
    const uint32_t saved_jsqr = adc->JSQR;
    const uint32_t saved_cr1  = adc->CR1;
    const uint32_t saved_cr2  = adc->CR2;
    LL_ADC_DisableIT_JEOS(adc);
    LL_ADC_INJ_StopConversionExtTrig(adc);
    LL_ADC_INJ_SetTriggerSource(adc, LL_ADC_INJ_TRIG_SOFTWARE);
    LL_ADC_INJ_SetSequencerLength(adc, LL_ADC_INJ_SEQ_SCAN_ENABLE_{{ inst.cs_found | length }}RANKS);
    {% for phase, ch in inst.analog.inj.items() if phase.startswith("cs")%} 
        LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_{{ loop.index }}, LL_ADC_CHANNEL_{{ ch[0] }});
    {% endfor %}

    //Start conversion in software
    for(int i = 0; i < sample_count; i++)
    {
        LL_ADC_INJ_StartConversionSWStart(adc);
        //Read here and accumulate
        while (!LL_ADC_IsActiveFlag_JEOS(adc)){}
        {% for phase, adc in inst.analog.inj.items() if phase.startswith("cs")%} 
            calibration_sum[{{loop.index0}}] += LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_{{ loop.index}});
        {% endfor %}
    }
    {% for phase, adc in inst.analog.inj.items() if phase.startswith("cs")%} 
        h->syncScale[FdMc3pSync_Cs{{phase[2:] | upper}}][1] = (calibration_sum[{{loop.index0}}]>>calibration_samples_log2);
    {% endfor %}

    //offset calibratin finished restore config we modified
    adc->CR1  = saved_cr1;
    adc->JSQR = saved_jsqr;
    adc->CR2  = saved_cr2;
    //End of hardware specific code
    h->mode = FdMc3pMode_None;
    FdMc3p_{{inst.name}}_WriteFloat(h);
}
void FdMc3p_{{inst.name}}_ReconfigurePwm(FdMc3pHandle *h)
{
    TIM_TypeDef *tim = TIM{{inst.pwm.tim}};
    LL_TIM_SetAutoReload(tim, __LL_TIM_CALC_ARR(PeriphBusClk(tim), 1, h->config.pwm_hz));
    LL_TIM_OC_SetDeadTime(tim, (uint8_t)(__LL_TIM_CALC_DEADTIME(PeriphBusClk(tim), LL_TIM_GetClockDivision(tim), h->config.deadtime_ns)));
    //GET TIMER MAX MAX VALUE TO BE USED FOR DUTY CALCULATION
    h->maxTimer = LL_TIM_GetAutoReload(tim);
    h->maxDuty = (uint16_t)((h->config.maxDuty) * 0x7FFF);
    h->minDuty = (uint16_t)((h->config.minDuty) * 0x7FFF);
    //compute deadtme compensation
    PrvMc3p_DtcUpdateComp(h);
}
void FdMc3p_{{inst.name}}_WritePhaseState(FdMc3pHandle *h, FdMc3pPhaseState state_u, FdMc3pPhaseState state_v, FdMc3pPhaseState state_w)
{
    TIM_TypeDef *tim = TIM{{inst.pwm.tim}};
    uint32_t ccer_shadow = tim->CCER;
    uint32_t ccmr1_shadow = tim->CCMR1;
    uint32_t ccmr2_shadow = tim->CCMR2;
    ccer_shadow |= (CC1E_MASK | CC2E_MASK | CC3E_MASK | CC1NE_MASK | CC2NE_MASK | CC3NE_MASK);  // Set all OCxNE & OCxE bits
    ccmr1_shadow &= ~(OC1M_MASK | OC2M_MASK);  // Clear OC1M bits & OC2M bits
    ccmr2_shadow &= ~OC3M_MASK;  // Clear OC3M bits

    {% for ch in ["u", "v", "w"] %}
    {% if inst.pwm[ch+"h"][1] == 1 %}
    switch(state_{{ch}})
    {//Channel 1 operations
        case(FdMc3pPhaseState_Float)  :   {ccmr1_shadow |= OC1M_INACTIVE; ccer_shadow  &= ~(CC1E_MASK); break;}
        case(FdMc3pPhaseState_LOn)   :   {ccmr1_shadow |= OC1M_INACTIVE; break;}
        case(FdMc3pPhaseState_HOn)   :   {ccmr1_shadow |= OC1M_ACTIVE;   break;}
        case(FdMc3pPhaseState_Comp)   :   {ccmr1_shadow |= OC1M_PWM;      break;}
        case(FdMc3pPhaseState_LPwm)  :   {ccmr1_shadow |= OC1M_PWM   ; ccer_shadow &=  ~(CC1E_MASK); break;}
        case(FdMc3pPhaseState_HPwm)  :   {ccmr1_shadow |= OC1M_PWM   ; ccer_shadow &=  ~(CC1NE_MASK); break;}
    }
    {% endif %}
    {% endfor %}

    {% for ch in ["u", "v", "w"] %}
    {% if inst.pwm[ch+"h"][1] == 2 %}
    switch(state_{{ch}})
    {//Channel 2 operations
        case(FdMc3pPhaseState_Float)  :   {ccmr1_shadow |= OC2M_INACTIVE; ccer_shadow  &= ~(CC2E_MASK); break;}
        case(FdMc3pPhaseState_LOn)   :   {ccmr1_shadow |= OC2M_INACTIVE; break;}
        case(FdMc3pPhaseState_HOn)   :   {ccmr1_shadow |= OC2M_ACTIVE;   break;}
        case(FdMc3pPhaseState_Comp)   :   {ccmr1_shadow |= OC2M_PWM;      break;}
        case(FdMc3pPhaseState_LPwm)  :   {ccmr1_shadow |= OC2M_PWM   ; ccer_shadow &=  ~(CC2E_MASK); break;}
        case(FdMc3pPhaseState_HPwm)  :   {ccmr1_shadow |= OC2M_PWM   ; ccer_shadow &=  ~(CC2NE_MASK); break;}
    }
    {% endif %}
    {% endfor %}

    {% for ch in ["u", "v", "w"] %}
    {% if inst.pwm[ch+"h"][1] == 3 %}
    switch(state_{{ch}})
    {//Channel 3 operations
        case(FdMc3pPhaseState_Float)  :   {ccmr2_shadow |= OC3M_INACTIVE; ccer_shadow  &= ~(CC3E_MASK); break;}
        case(FdMc3pPhaseState_LOn)   :   {ccmr2_shadow |= OC3M_INACTIVE; break;}
        case(FdMc3pPhaseState_HOn)   :   {ccmr2_shadow |= OC3M_ACTIVE;   break;}
        case(FdMc3pPhaseState_Comp)   :   {ccmr2_shadow |= OC3M_PWM;      break;}
        case(FdMc3pPhaseState_LPwm)  :   {ccmr2_shadow |= OC3M_PWM   ; ccer_shadow &=  ~(CC3E_MASK); break;}
        case(FdMc3pPhaseState_HPwm)  :   {ccmr2_shadow |= OC3M_PWM   ; ccer_shadow &=  ~(CC3NE_MASK); break;}
    }
    {% endif %}
    {% endfor %}

    tim->CCER = ccer_shadow;
    tim->CCMR1 = ccmr1_shadow;
    tim->CCMR2 = ccmr2_shadow;
    tim->EGR |= TIM_EGR_COMG;
}
void FdMc3p_{{inst.name}}_WritePhaseDuty(FdMc3pHandle *h, int16_t dutyU, int16_t dutyV, int16_t dutyW)
{
    TIM_TypeDef *tim = TIM{{inst.pwm.tim}};
    uint32_t compareU = ((uint32_t)MC3P_SATURATE(dutyU, h->minDuty, h->maxDuty) * h->maxTimer) >> 15;
    uint32_t compareV = ((uint32_t)MC3P_SATURATE(dutyV, h->minDuty, h->maxDuty) * h->maxTimer) >> 15;
    uint32_t compareW = ((uint32_t)MC3P_SATURATE(dutyW, h->minDuty, h->maxDuty) * h->maxTimer) >> 15;
    tim->CCR{{inst.pwm["uh"][1]}} = compareU;
    tim->CCR{{inst.pwm["vh"][1]}} = compareV;
    tim->CCR{{inst.pwm["wh"][1]}} = compareW;
}
void FdMc3p_{{inst.name}}_read_sync(FdMc3pHandle *h, void *data)
{
    ADC_TypeDef *adc = ADC{{ inst.analog.adc }};
    uint8_t is_svm = IS_SVM_SECTOR(h->sectorLast);
    uint8_t is_trap = IS_TRAP_SECTOR(h->sectorLast);
    ((FdMc3pTrapData *)(data))->vBus_vsQ31 = h->rankScale[0][0] * (LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_1) - h->rankScale[0][1]);
    if (is_svm)
    {
        FdMc3pSvmData *svm = (FdMc3pSvmData *)data;
        //Read Current Sensors in the same order we put the rank
        {% if mc3p.current_sensing.topology in ["three_shunt","two_shunt","two_inline"] %}
        {% for phase, adc in inst.analog.inj.items() if phase.startswith("cs")%} 
        svm->c{{ phase[2:] | upper }}_csQ31 = h->rankScale[{{loop.index}}][0] * (LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_{{ loop.index + 1 }}) - h->rankScale[{{ loop.index }}][1]);
        {% endfor %}
        //Infer the third phase current from the 2x measured current for double measurements
        {% if mc3p.current_sensing.topology in ["two_shunt","two_inline"] %}
        svm->c{{inst.cs_missing[0]  | upper }}_csQ31 = -svm->c{{inst.cs_found[0]  | upper }}_csQ31 -svm->c{{inst.cs_found[1]  | upper }}_csQ31;
        {% endif %}
        {% elif mc3p.current_sensing.topology == "single_shunt" %}
        
        {% endif %}
        // Dead-time compensation
        PrvMc3p_DtcUpdatePolarity(h, svm->cU_csQ31, (1 << 0));
        PrvMc3p_DtcUpdatePolarity(h, svm->cV_csQ31, (1 << 1));
        PrvMc3p_DtcUpdatePolarity(h, svm->cW_csQ31, (1 << 2));
    }
    else if (is_trap)
    {
        FdMc3pTrapData *trap = (FdMc3pTrapData *)data;
        trap->vBemf_vsQ31 = h->rankScale[1][0] * (LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_2) - h->rankScale[1][1]);
        trap->cBus_csQ31 =  h->rankScale[2][0] * (LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_3) - h->rankScale[2][1]);
        PrvMc3p_DtcUpdatePolarity(h, trap->cBus_csQ31, (1 << 3));
    }
}
void FdMc3p_{{inst.name}}_BgStartConv(FdMc3pHandle *h)
{
    LL_ADC_REG_StartConversionSWStart(ADC{{inst.analog.adc}});
}

//Create static instances here of fd mc3p
__attribute__((weak)) void FdMc3p_{{inst.name}}_SyncPostScanCallback(void){}
__attribute__((weak)) void FdMc3p_{{inst.name}}_BgPostScanCallback(void){}
FdMc3pHandle {{inst.name}};

void {{inst.name}}_bg_dma_irqhandler()
{
    DMA_TypeDef *bg_dma = {{bg_dma.dma}};
    if (LL_DMA_IsActiveFlag_TC{{bg_dma.irq_flag_num}}(bg_dma)) {
        LL_DMA_ClearFlag_TC{{bg_dma.irq_flag_num}}(bg_dma);
        FdMc3p_{{inst.name}}_BgPostScanCallback();
    }
}
void {{inst.name}}_adc_irqhandler()
{
    ADC_TypeDef *adc = ADC{{ inst.analog.adc }};
    if (LL_ADC_IsActiveFlag_JEOS(adc)) {
        LL_ADC_ClearFlag_JEOS(adc);
        FdMc3p_{{ inst.name }}_SyncPostScanCallback();
    }
}
void {{inst.name}}_tim_up_irqhandler()
{
    TIM_TypeDef *tim = TIM{{inst.pwm.tim}};
    if (LL_TIM_IsActiveFlag_UPDATE(tim)) {
        LL_TIM_ClearFlag_UPDATE(tim);
    }
}

{% endfor %}

