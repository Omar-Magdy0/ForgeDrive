#pragma once
#include <stdint.h>


uint16_t FdAdc_SampleSingle(uint32_t channel);
void FdAdc_Calibrate();
extern uint8_t FdAdc_CommonInitialized;
