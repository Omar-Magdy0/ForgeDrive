#pragma once
#include <stdint.h>


uint16_t adc_Sample_Single(uint32_t channel);
void adc_calibrate();
extern uint8_t adc_common_initialized;
