
#include "fd_mc3p.h"
#include "fd_adc.h"
#include <string.h>
#include "fd_drv.h"
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

#define ADC_GpioInit(port, pin)do{\
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};\
    GPIO_InitStruct.Pin = pin;\
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;\
    LL_GPIO_Init(port, &GPIO_InitStruct);\
}while(0)

#define PWM_GpioInit(port, pin, active)do{\
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};\
    GPIO_InitStruct.Pin = pin;\
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;\
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;\
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;\
    GPIO_InitStruct.Pull = active?LL_GPIO_PULL_DOWN : LL_GPIO_PULL_UP;\
    GPIO_InitStruct.Alternate = LL_GPIO_AF_1;\
    LL_GPIO_Init(port, &GPIO_InitStruct);\
}while(0)


#define COPY_SCALE_SIGNED(dst, src, sign)do { \
    h->rank_scale[dst][0] = (sign == -1)? -h->sync_scale[src][0] : h->sync_scale[src][0]; \
    h->rank_scale[dst][1] = h->sync_scale[src][1]; \
} while (0)
#define COPY_SCALE(dst, src)  COPY_SCALE_SIGNED(dst, src, 1)

{% for inst in mc3p.instances %}

static inj_rank_t {{inst.name}}_trapVs_table[6] = 
{
    {% for entry in inst.trapVs_vector %}
    { LL_ADC_CHANNEL_{{ entry.channel }}, FD_MC3P_VS{{ entry.phase | upper }}, {{entry.sign}}  },
    {% endfor %}
};
static inj_rank_t {{inst.name}}_trapCs_table[6] = 
{
    {% for entry in inst.trapCs_vector %}
    { LL_ADC_CHANNEL_{{ entry.channel }}, FD_MC3P_CS{{ entry.phase | upper }}, {{entry.sign}} }, 
    {% endfor %}
};
void {{inst.name}}_gpio_init()
{     
    //Initialize all MC3P instance Gpio here
    {% for pwmSignal in ["uh","vh","wh","ul","vl","wl"] %}
    PWM_GpioInit( GPIO{{inst.tim[pwmSignal].port}}, LL_GPIO_PIN_{{inst.tim[pwmSignal].pin}}, {{inst.tim[pwmSignal].active}});
    __FDGEN__CLK_EN(GPIO{{inst.tim[pwmSignal].port}})
    {% endfor %}
    //Initialize adc pins (inj channels)
    {% for name, ch in inst.adc.inj.items() %}
    {% if ch.port is defined and ch.pin is defined %}
    ADC_GpioInit( GPIO{{ch.port}}, LL_GPIO_PIN_{{ch.pin}});
    __FDGEN__CLK_EN(GPIO{{ch.port}})
    {% endif %}
    {% endfor %}
    {% for name, ch in inst.adc.bg.items() %}
    {% if ch.port is defined and ch.pin is defined %}
    __FDGEN__CLK_EN(GPIO{{ch.port}})
    {% endif %}
    {% endfor %}
}
static void {{inst.name}}_tim_init(fd_mc3p_handle_t *h){
    //ENABLE Peripheral CLOCK
    TIM_TypeDef * const tim = TIM{{inst.tim.num}};
    //===================================================== 
    // BASIC TIMER CONFIGURATION
    //=====================================================
    LL_TIM_InitTypeDef timInit =
    {
        .Prescaler = 0,
        .CounterMode = LL_TIM_COUNTERMODE_CENTER_UP,
        .Autoreload = __LL_TIM_CALC_ARR(get_periph_bus_clk(tim), 1, h->config.pwm_Hz),
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
        .DeadTime = (uint8_t)(__LL_TIM_CALC_DEADTIME(get_periph_bus_clk(tim), LL_TIM_GetClockDivision(tim), h->config.deadtime_nS)),
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
        .OCPolarity = {{ inst.tim[ch + "h"].active }} ? LL_TIM_OCPOLARITY_HIGH : LL_TIM_OCPOLARITY_LOW,
        .OCNPolarity = {{ inst.tim[ch + "l"].active }} ? LL_TIM_OCPOLARITY_HIGH : LL_TIM_OCPOLARITY_LOW,
        .OCIdleState = {{ inst.tim[ch + "h"].active }} ? LL_TIM_OCIDLESTATE_LOW : LL_TIM_OCIDLESTATE_HIGH,
        .OCNIdleState = {{ inst.tim[ch + "l"].active }} ? LL_TIM_OCIDLESTATE_LOW : LL_TIM_OCIDLESTATE_HIGH
    };
    LL_TIM_OC_Init(tim, LL_TIM_CHANNEL_CH{{inst.tim[ch + "h"].ch}}, &{{ch}}_tim_OCInit);
    LL_TIM_OC_EnablePreload(tim, LL_TIM_CHANNEL_CH{{inst.tim[ch + "h"].ch}});
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
    h->timer_max_q15 = LL_TIM_GetAutoReload(tim);
    // Finally enable the timer (Configuration done)
    LL_TIM_EnableAllOutputs(tim);
    LL_TIM_EnableCounter(tim);
}
void {{inst.name}}_dma_int_init(fd_mc3p_handle_t *h)
{
    // Enable DMA clock
    DMA_TypeDef *bg_dma = DMA{{inst.dma["bg"].num}};
    uint32_t bg_stream = LL_DMA_STREAM_{{inst.dma["bg"].stream}};
    uint32_t bg_channel = LL_DMA_CHANNEL_{{inst.dma["bg"].channel}};
     uint32_t bg_prio = LL_DMA_PRIORITY_{{inst.dma["bg"].prio | upper}};
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
    LL_DMA_SetPeriphAddress(bg_dma, bg_stream, (uint32_t)&(ADC{{inst.adc.num}}->DR));
    
    // Set memory address (double buffer start)
    LL_DMA_SetMemoryAddress(bg_dma, bg_stream, (uint32_t)h->bg_data[0]);
    
    // Set data length (total samples across both buffers)
    LL_DMA_SetDataLength(bg_dma, bg_stream, h->bg_channels);

    // Enable DMA transfer complete and half transfer interrupts
    LL_DMA_EnableIT_TC(bg_dma, bg_stream);
    
    // Enable DMA stream
    LL_DMA_EnableStream(bg_dma, bg_stream);
    
    // Enable JEOS (Injected End Of Sequence) interrupt in ADC
    LL_ADC_EnableIT_JEOS(ADC{{inst.adc.num}});
}
void {{inst.name}}_adc_init(fd_mc3p_handle_t *h)
{
    ADC_TypeDef *adc = ADC{{ inst.adc.num }};
    if (!adc_common_initialized)
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
    adc_calibrate();
    
    //bg sequencer and rank config
    {% if inst.adc.bg | length > 0 %}

    /* Configure regular ADC sequence */
    LL_ADC_REG_InitTypeDef adcRegInitStruct = {
        .TriggerSource = LL_ADC_REG_TRIG_SOFTWARE,
        .SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_{{ inst.adc.bg | length }}RANKS,
        .SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE,
        .ContinuousMode = LL_ADC_REG_CONV_CONTINUOUS,
        .DMATransfer = LL_ADC_REG_DMA_TRANSFER_UNLIMITED
    };

    LL_ADC_REG_Init(adc, &adcRegInitStruct);

    /* Configure channel ranks and sampling times */
    {% for signal, config in inst.adc.bg.items() %} 
    LL_ADC_REG_SetSequencerRanks(adc, LL_ADC_REG_RANK_{{ loop.index }}, LL_ADC_CHANNEL_{{ config.channel }});
    LL_ADC_SetChannelSamplingTime(adc, LL_ADC_CHANNEL_{{ config.channel }}, LL_ADC_SAMPLINGTIME_{{ config.sampling_cycles | default("56") }}CYCLES );
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
    {% for signal, config in inst.adc.inj.items()  %}
    LL_ADC_SetChannelSamplingTime(adc, LL_ADC_CHANNEL_{{ config.channel }},  LL_ADC_SAMPLINGTIME_{{ config.sampling_cycles | default("56") }}CYCLES );
    {% endfor %}

    LL_ADC_INJ_Init(adc, &adcInjInitStruct);
    //ADC CONFIGURATIONS AND LINKAGE TO TIM1TRGO
    LL_ADC_INJ_StartConversionExtTrig(adc, LL_ADC_INJ_TRIG_EXT_RISING);
    // Enable ADC interrupts
    LL_ADC_EnableIT_JEOS(adc);  // Injected sequence end interrupt  
}
void pv_mc3p_{{inst.name}}_hw_init(fd_mc3p_handle_t *h)
{
    {{inst.name}}_gpio_init();
    {{inst.name}}_tim_init(h);
    {{inst.name}}_dma_int_init(h);
    {{inst.name}}_adc_init(h);
}

//Mode and sector changes
void {{inst.name}}_svm_mode(fd_mc3p_handle_t *h)
{
    //do needed pwm and adc updates
    ADC_TypeDef *adc = ADC{{ inst.adc.num }};
    fd_mc3p_{{inst.name}}_write_phase_state(h, FD_MC3P_PHASE_COMP, FD_MC3P_PHASE_COMP, FD_MC3P_PHASE_COMP);
    LL_ADC_INJ_SetSequencerLength(adc, LL_ADC_INJ_SEQ_SCAN_ENABLE_4RANKS);
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_1, {{inst.adc.inj["vsbus"].channel}});
    COPY_SCALE(0, FD_MC3P_VSBUS);
    //enable all current sensors
    {% for phase, adc in inst.adc.inj.items() if phase.startswith("cs")%} 
        LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_{{ loop.index + 1 }}, LL_ADC_CHANNEL_{{ adc.channel }});
        COPY_SCALE({{ loop.index }}, FD_MC3P_CS{{ phase[2:] | upper }});
    {% endfor %}
}
void {{inst.name}}_trap_mode(fd_mc3p_handle_t *h)
{
    //do needed pwm and adc updates
    ADC_TypeDef *adc = ADC{{ inst.adc.num }};
    LL_ADC_INJ_SetSequencerLength(adc, LL_ADC_INJ_SEQ_SCAN_ENABLE_4RANKS);
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_1, {{inst.adc.inj["vsbus"].channel}});
    COPY_SCALE(0, FD_MC3P_VSBUS);
    {% if mc3p.current_sensing.topology == "single_shunt" %}
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_3, {{inst.adc.inj["csbus"].channel}});
    COPY_SCALE_SIGNED(2, FD_MC3P_CSBUS, -1);
    {% endif %}
}
void {{inst.name}}_trap_sector(fd_mc3p_handle_t *h, fd_mc3p_sector_t sector)
{
    //do needed pwm and adc updates
    ADC_TypeDef *adc = ADC{{ inst.adc.num }};
    const inj_rank_t *trapCs_map = &{{inst.name}}_trapCs_table[sector - 1];
    const inj_rank_t *trapVs_map = &{{inst.name}}_trapVs_table[sector - 1];
    const mc3p_trap_sector_map_t *pwm_map = &trap_table[sector - 1];
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_3, trapCs_map->channel);
    COPY_SCALE(1, trapVs_map->scale_idx);
    {% if not mc3p.current_sensing.topology == "single_shunt" %}
    fd_mc3p_{{inst.name}}_write_phase_state(h, pwm_map->phase_state[0], pwm_map->phase_state[1], pwm_map->phase_state[2]);
    LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_3, trapCs_map->channel);
    COPY_SCALE_SIGNED(2, trapCs_map->scale_idx, trapCs_map->sign);
    {% endif %}
}
void {{inst.name}}_svm_sector(fd_mc3p_handle_t *h, fd_mc3p_sector_t sector)
{
    TIM_TypeDef * const tim = TIM{{inst.tim.num}};
    ADC_TypeDef *adc = ADC{{ inst.adc.num }};
    {% if mc3p.current_sensing.topology == "three_shunt" %}
    {% elif mc3p.current_sensing.topology == "two_shunt" %}
    {% elif mc3p.current_sensing.topology == "two_inline" %}
    {% elif mc3p.current_sensing.topology == "single_shunt" %}
    {% endif %}
}

void pv_mc3p_{{inst.name}}_sector_update(fd_mc3p_handle_t *h, fd_mc3p_sector_t sector)
{//This method is called on sector changes, mode changes are inferred from sector
    //Check for mode changes....
    fd_mc3p_mode_t cm = fd_mc3p_sector_to_mode(sector);
    if(cm != h->mode)//mode changed... call mode update functions
    {
        switch (cm)
        {
        case FD_MC3P_MODE_TRAP:
            {{inst.name}}_trap_mode(h);
            break;
        case FD_MC3P_MODE_SVM:
            {{inst.name}}_svm_mode(h);
            break;
        default:
            break;
        }
        h->mode = cm;
    }
    //update sector
    switch (h->mode)
    {
    case FD_MC3P_MODE_TRAP:
        {{inst.name}}_trap_sector(h, sector);
        break;
    case FD_MC3P_MODE_SVM:
        {{inst.name}}_svm_sector(h, sector);
        break;
    default:
        break;
    }

    h->sector_last = sector;
}
void pv_mc3p_{{inst.name}}_offset_calibration(fd_mc3p_handle_t *h)
{
    // Set all phases to Low (0,0,0)
    // Set mode to calibration
    fd_mc3p_{{inst.name}}_write_float(h);
    h->mode = FD_MC3P_MODE_CALIB;
    const uint32_t calibration_samples_log2 = {{mc3p["current_sensing"].calibration_samples_log2 | default("10")}};
    const uint32_t sample_count = 1U << calibration_samples_log2;
    uint32_t calibration_sum[3] = {0U, 0U, 0U};
    //Lets start by doing offset calibration for current sensors
    //Hardware Specific code
    //Disable the JEOS IRQ temporarily and timer triggering, we will be doing SW triggering for now, this is a blocking method
    // Save the ADC configuration that this routine will modify.
    ADC_TypeDef *adc = ADC{{ inst.adc.num }};
    const uint32_t saved_jsqr = adc->JSQR;
    const uint32_t saved_cr1  = adc->CR1;
    const uint32_t saved_cr2  = adc->CR2;
    LL_ADC_DisableIT_JEOS(adc);
    LL_ADC_INJ_StopConversionExtTrig(adc);
    LL_ADC_INJ_SetTriggerSource(adc, LL_ADC_INJ_TRIG_SOFTWARE);
    LL_ADC_INJ_SetSequencerLength(adc, LL_ADC_INJ_SEQ_SCAN_ENABLE_{{ inst.cs_found | length }}RANKS);
    {% for phase, adc in inst.adc.inj.items() if phase.startswith("cs")%} 
        LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_{{ loop.index }}, LL_ADC_CHANNEL_{{ adc.channel }});
    {% endfor %}

    //Start conversion in software
    for(int i = 0; i < sample_count; i++)
    {
        LL_ADC_INJ_StartConversionSWStart(adc);
        //Read here and accumulate
        while (!LL_ADC_IsActiveFlag_JEOS(adc)){}
        {% for phase, adc in inst.adc.inj.items() if phase.startswith("cs")%} 
            calibration_sum[{{loop.index0}}] += LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_{{ loop.index}});
        {% endfor %}
    }
    {% for phase, adc in inst.adc.inj.items() if phase.startswith("cs")%} 
        h->sync_scale[FD_MC3P_CS{{phase[2:] | upper}}][1] = (calibration_sum[{{loop.index0}}]>>calibration_samples_log2);
    {% endfor %}

    //offset calibratin finished restore config we modified
    adc->CR1  = saved_cr1;
    adc->JSQR = saved_jsqr;
    adc->CR2  = saved_cr2;
    //End of hardware specific code
    h->mode = FD_MC3P_MODE_NONE;
    fd_mc3p_{{inst.name}}_write_float(h);
}
void fd_mc3p_{{inst.name}}_reconfigure_pwm(fd_mc3p_handle_t *h)
{
    TIM_TypeDef *tim = TIM{{inst.tim.num}};
    LL_TIM_SetAutoReload(tim, __LL_TIM_CALC_ARR(get_periph_bus_clk(tim), 1, h->config.pwm_Hz));
    LL_TIM_OC_SetDeadTime(tim, (uint8_t)(__LL_TIM_CALC_DEADTIME(get_periph_bus_clk(tim), LL_TIM_GetClockDivision(tim), h->config.deadtime_nS)));
    //GET TIMER MAX MAX VALUE TO BE USED FOR DUTY CALCULATION
    h->timer_max_q15 = LL_TIM_GetAutoReload(tim);
    h->duty_max_q15 = (uint16_t)((h->config.duty_max) * 0x7FFF);
    h->duty_min_q15 = (uint16_t)((h->config.duty_min) * 0x7FFF);
    //compute deadtme compensation
    #ifdef FD_MC3P_DTC_ACTIVE
    h->dtc_comp_q15 = (uint16_t)(((float)h->config.deadtime_nS*h->config.pwm_Hz/1e9)*INT16_MAX + 0.5);
    #endif
}
void fd_mc3p_{{inst.name}}_write_phase_state(fd_mc3p_handle_t *h, fd_mc3p_phase_state_t state_u, fd_mc3p_phase_state_t state_v, fd_mc3p_phase_state_t state_w)
{
    TIM_TypeDef *tim = TIM{{inst.tim.num}};
    uint32_t ccer_shadow = tim->CCER;
    uint32_t ccmr1_shadow = tim->CCMR1;
    uint32_t ccmr2_shadow = tim->CCMR2;
    ccer_shadow |= (CC1E_MASK | CC2E_MASK | CC3E_MASK | CC1NE_MASK | CC2NE_MASK | CC3NE_MASK);  // Set all OCxNE & OCxE bits
    ccmr1_shadow &= ~(OC1M_MASK | OC2M_MASK);  // Clear OC1M bits & OC2M bits
    ccmr2_shadow &= ~OC3M_MASK;  // Clear OC3M bits

    {% for ch in ["u", "v", "w"] %}
    {% if inst.tim[ch+"h"].ch == 1 %}
    switch(state_{{ch}})
    {//Channel 1 operations
        case(FD_MC3P_PHASE_FLOAT)  :   {ccmr1_shadow |= OC1M_INACTIVE; ccer_shadow  &= ~(CC1E_MASK); break;}
        case(FD_MC3P_PHASE_L_ON)   :   {ccmr1_shadow |= OC1M_INACTIVE; break;}
        case(FD_MC3P_PHASE_H_ON)   :   {ccmr1_shadow |= OC1M_ACTIVE;   break;}
        case(FD_MC3P_PHASE_COMP)   :   {ccmr1_shadow |= OC1M_PWM;      break;}
        case(FD_MC3P_PHASE_L_PWM)  :   {ccmr1_shadow |= OC1M_PWM   ; ccer_shadow &=  ~(CC1E_MASK); break;}
        case(FD_MC3P_PHASE_H_PWM)  :   {ccmr1_shadow |= OC1M_PWM   ; ccer_shadow &=  ~(CC1NE_MASK); break;}
    }
    {% endif %}
    {% endfor %}

    {% for ch in ["u", "v", "w"] %}
    {% if inst.tim[ch+"h"].ch == 2 %}
    switch(state_{{ch}})
    {//Channel 2 operations
        case(FD_MC3P_PHASE_FLOAT)  :   {ccmr1_shadow |= OC2M_INACTIVE; ccer_shadow  &= ~(CC2E_MASK); break;}
        case(FD_MC3P_PHASE_L_ON)   :   {ccmr1_shadow |= OC2M_INACTIVE; break;}
        case(FD_MC3P_PHASE_H_ON)   :   {ccmr1_shadow |= OC2M_ACTIVE;   break;}
        case(FD_MC3P_PHASE_COMP)   :   {ccmr1_shadow |= OC2M_PWM;      break;}
        case(FD_MC3P_PHASE_L_PWM)  :   {ccmr1_shadow |= OC2M_PWM   ; ccer_shadow &=  ~(CC2E_MASK); break;}
        case(FD_MC3P_PHASE_H_PWM)  :   {ccmr1_shadow |= OC2M_PWM   ; ccer_shadow &=  ~(CC2NE_MASK); break;}
    }
    {% endif %}
    {% endfor %}

    {% for ch in ["u", "v", "w"] %}
    {% if inst.tim[ch+"h"].ch == 3 %}
    switch(state_{{ch}})
    {//Channel 3 operations
        case(FD_MC3P_PHASE_FLOAT)  :   {ccmr2_shadow |= OC3M_INACTIVE; ccer_shadow  &= ~(CC3E_MASK); break;}
        case(FD_MC3P_PHASE_L_ON)   :   {ccmr2_shadow |= OC3M_INACTIVE; break;}
        case(FD_MC3P_PHASE_H_ON)   :   {ccmr2_shadow |= OC3M_ACTIVE;   break;}
        case(FD_MC3P_PHASE_COMP)   :   {ccmr2_shadow |= OC3M_PWM;      break;}
        case(FD_MC3P_PHASE_L_PWM)  :   {ccmr2_shadow |= OC3M_PWM   ; ccer_shadow &=  ~(CC3E_MASK); break;}
        case(FD_MC3P_PHASE_H_PWM)  :   {ccmr2_shadow |= OC3M_PWM   ; ccer_shadow &=  ~(CC3NE_MASK); break;}
    }
    {% endif %}
    {% endfor %}

    tim->CCER = ccer_shadow;
    tim->CCMR1 = ccmr1_shadow;
    tim->CCMR2 = ccmr2_shadow;
    tim->EGR |= TIM_EGR_COMG;
}
void fd_mc3p_{{inst.name}}_write_phase_duty(fd_mc3p_handle_t *h, int16_t dutyu_q15, int16_t dutyv_q15, int16_t dutyw_q15)
{
    TIM_TypeDef *tim = TIM{{inst.tim.num}};
    uint32_t compare_u = ((uint32_t)SATURATE(dutyu_q15, h->duty_min_q15, h->duty_max_q15) * h->timer_max_q15) >> 15;
    uint32_t compare_v = ((uint32_t)SATURATE(dutyv_q15, h->duty_min_q15, h->duty_max_q15) * h->timer_max_q15) >> 15;
    uint32_t compare_w = ((uint32_t)SATURATE(dutyw_q15, h->duty_min_q15, h->duty_max_q15) * h->timer_max_q15) >> 15;
    tim->CCR{{inst.tim["uh"].ch}} = compare_u;
    tim->CCR{{inst.tim["vh"].ch}} = compare_v;
    tim->CCR{{inst.tim["wh"].ch}} = compare_w;
}
void fd_mc3p_{{inst.name}}_read_sync(fd_mc3p_handle_t *h, void *data)
{
    ADC_TypeDef *adc = ADC{{ inst.adc.num }};
#define DTC_UPDATE_POLARITY(current, bit)         \
    do                                            \
    {                                             \
        if ((current) > (int32_t)FD_MC3P_DTC_CTHRESH)       \
        {                                         \
            h->dtc_state |= (bit);             \
        }                                         \
        else if ((current) < -(int32_t)FD_MC3P_DTC_CTHRESH) \
        {                                         \
            h->dtc_state &= ~(bit);            \
        }                                         \
    } while (0)

    uint8_t is_svm = IS_SVM_SECTOR(h->sector_last);
    uint8_t is_trap = IS_TRAP_SECTOR(h->sector_last);
    ((fd_mc3p_trap_data_t *)(data))->vbus_q31 = h->rank_scale[0][0] * (LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_1) - h->rank_scale[0][1]);
    if (is_svm)
    {
        fd_mc3p_svm_data_t *svm = (fd_mc3p_svm_data_t *)data;
        //Read Current Sensors in the same order we put the rank
        {% if mc3p.current_sensing.topology in ["three_shunt","two_shunt","two_inline"] %}
        {% for phase, adc in inst.adc.inj.items() if phase.startswith("cs")%} 
        svm->c{{ phase[2:] }}_q31 = h->rank_scale[{{loop.index}}][0] * (LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_{{ loop.index + 1 }}) - h->rank_scale[{{ loop.index }}][1]);
        {% endfor %}
        //Infer the third phase current from the 2x measured current for double measurements
        {% if mc3p.current_sensing.topology in ["two_shunt","two_inline"] %}
        svm->c{{inst.cs_missing[0]}}_q31 = -svm->c{{inst.cs_found[0]}}_q31 -svm->c{{inst.cs_found[1]}}_q31;
        {% endif %}
        {% elif mc3p.current_sensing.topology == "single_shunt" %}
        
        {% endif %}
        // Dead-time compensation
        #ifdef FD_MC3P_DTC_ACTIVE
        DTC_UPDATE_POLARITY(svm->cu_q31, (1 << 0));
        DTC_UPDATE_POLARITY(svm->cv_q31, (1 << 1));
        DTC_UPDATE_POLARITY(svm->cw_q31, (1 << 2));
        #endif
    }
    else if (is_trap)
    {
        fd_mc3p_trap_data_t *trap = (fd_mc3p_trap_data_t *)data;
        trap->vbemf_q31 = h->rank_scale[1][0] * (LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_2) - h->rank_scale[1][1]);
        trap->cbus_q31 =  h->rank_scale[2][0] * (LL_ADC_INJ_ReadConversionData12(adc, LL_ADC_INJ_RANK_3) - h->rank_scale[2][1]);
        #ifdef FD_MC3P_DTC_ACTIVE
        DTC_UPDATE_POLARITY(trap->cbus_q31 , (1 << 3));
        #endif
    }
}
void fd_mc3p_{{inst.name}}_bg_startConv(fd_mc3p_handle_t *h)
{
    LL_ADC_REG_StartConversionSWStart(ADC{{inst.adc.num}});
}

//Create static instances here of fd mc3p
__attribute__((weak)) void fd_mc3p_{{inst.name}}_sync_postScanCallback(void){}
__attribute__((weak)) void fd_mc3p_{{inst.name}}_bg_postScanCallback(void){}
fd_mc3p_handle_t {{inst.name}};

void {{inst.name}}_bg_dma_irqhandler()
{
    DMA_TypeDef *bg_dma = DMA{{inst.dma["bg"].num}};
    if (LL_DMA_IsActiveFlag_TC{{inst.dma["bg"].stream}}(bg_dma)) {
        LL_DMA_ClearFlag_TC{{inst.dma["bg"].stream}}(bg_dma);
        fd_mc3p_{{inst.name}}_bg_postScanCallback();
    }
}
void {{inst.name}}_adc_irqhandler()
{
    ADC_TypeDef *adc = ADC{{ inst.adc.num }};
    if (LL_ADC_IsActiveFlag_JEOS(adc)) {
        LL_ADC_ClearFlag_JEOS(adc);
        fd_mc3p_{{ inst.name }}_sync_postScanCallback();
    }
}
void {{inst.name}}_tim_up_irqhandler()
{
    TIM_TypeDef *tim = TIM{{inst.tim.num}};
    if (LL_TIM_IsActiveFlag_UPDATE(tim)) {
        LL_TIM_ClearFlag_UPDATE(tim);
    }
}

__FDGEN__CLK_EN(TIM{{inst.tim.num}})
__FDGEN__CLK_EN(DMA{{inst.dma["bg"].num}})
__FDGEN__CLK_EN(ADC1)

__FDGEN__IRQ_EN(TIM{{inst.tim.num}}_UP, 1)
__FDGEN__IRQ_EN(ADC, 0)
__FDGEN__IRQ_EN(DMA{{inst.dma["bg"].num}}_Stream{{inst.dma["bg"].stream}}, 5)
__FDGEN__IRQ_BIND(TIM{{inst.tim.num}}_UP, {{inst.name}}_tim_up_irqhandler)
__FDGEN__IRQ_BIND(ADC,{{inst.name}}_adc_irqhandler)
__FDGEN__IRQ_BIND(DMA{{inst.dma["bg"].num}}_Stream{{inst.dma["bg"].stream}}, {{inst.name}}_bg_dma_irqhandler)

{% endfor %}

