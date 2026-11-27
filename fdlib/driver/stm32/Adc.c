#include "Adc.h"
#include "stm32{{device.family}}xx_ll_adc.h"
#include "stm32{{device.family}}xx_ll_tim.h"
#include "stm32{{device.family}}xx_ll_rcc.h"
#include "stm32{{device.family}}xx_ll_tim.h"
#include "stm32{{device.family}}xx_ll_bus.h"
#include "stm32{{device.family}}xx_ll_gpio.h"
#include "Drv.h"


#define VREFIN_CAL_ADDR (uint16_t *)0x1FFF7A2A
#define VREFIN_CAL_VOLTAGE 3.3f
uint8_t FdAdc_CommonInitialized = 0;

//adc calibration sequence
uint16_t FdAdc_SampleSingle(uint32_t channel)
{
    // Save current configuration
    uint32_t old_sqr1 = ADC1->SQR1;
    uint32_t old_sqr2 = ADC1->SQR2;
    uint32_t old_sqr3 = ADC1->SQR3;
    
    // Configure for single conversion
    // Configure for single conversion USING LL FUNCTION
    LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_12B);
    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE); // 1 conversion
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, channel);   // Handles conversion 
    
    // Start conversion
    LL_ADC_REG_StartConversionSWStart(ADC1);
    while (!LL_ADC_IsActiveFlag_EOCS(ADC1));
    uint16_t result = LL_ADC_REG_ReadConversionData12(ADC1);
    
    // Restore original configuration
    ADC1->SQR1 = old_sqr1;
    ADC1->SQR2 = old_sqr2;
    ADC1->SQR3 = old_sqr3;
    
    return result;
}

// TODO: External reference voltage adc calibration support
void FdAdc_Calibrate()
{
    uint32_t sum = 0;
    const uint8_t num_samples = 30;

    #ifdef FDADC_VREFEXT
    ADC_GpioInit(); //initialize vref gpio
    volatile uint32_t wait = 10000;
    while(wait){wait = wait - 1;};

    // Set longer sampling time for VREFINT
    LL_ADC_SetChannelSamplingTime(, , LL_ADC_SAMPLINGTIME_480CYCLES);
    (adc_ref_V) = externalRefVoltage_V;

    #else//Internal reference based calibration
    
    ADC->CCR |= ADC_CCR_TSVREFE_Msk; //Enable VREFINT and internalTEMP sensor
    // Wait for internal references to stabilize (IMPORTANT!)
    volatile uint32_t wait = 10000;
    while(wait){wait = wait - 1;};

    // Set longer sampling time for VREFINT
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SAMPLINGTIME_480CYCLES);
    //Discard first sample
    FdAdc_SampleSingle(LL_ADC_CHANNEL_VREFINT);
    for (int i = 0; i < num_samples; i++) {
        sum += FdAdc_SampleSingle(LL_ADC_CHANNEL_VREFINT);
    }
    uint32_t inRef_div_adcRef = sum / num_samples;

    float internal_ref_V = 0;
    internal_ref_V = (*VREFIN_CAL_ADDR / (float)((1<<12) - 1) ) * VREFIN_CAL_VOLTAGE;

    // Calculate actual ADC reference voltage (your supply voltage)
    (DRV.adcRefVlt) = internal_ref_V * ((1<<12) - 1) / (float)inRef_div_adcRef;
    #endif
    (DRV.adc2Vlt) = (DRV.adcRefVlt) / (float)((1<<12) - 1);
}

