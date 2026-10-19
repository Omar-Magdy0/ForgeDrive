#ifndef DRV_CONF_H
#define DRV_CONF_H

//########################################################################
// FIXED HARDWARE MAPPING FOR STM32F4xx Series MCU's
//########################################################################
// UART1 for Serial (B6, B7)
// SWD (A13, A14)
// USB1 (A11, A12)
// CAN1 (B8, B9) (if exists (doesnt exist for F401, F411))
// TIM1 for complementary PWM generation (A8, A9, A10, B13, B14, B15)
// TIM2 for Hall sensor interface / sensorless commutation timing (mocked triggers) (A15, B3, B10)
// TIM3 for encoder interface(B4, B5)

// ANALOG PINS(A0, A1, A2, A3, A4, A5, A6, A7, B0, B1, C0, C1, C2, C3, C4, C5)  
//Will Need 9 Pins Min For the Case of Triple Shunt + Phase Voltages + Bus Voltage + 1 Temperature + 1 Throttle

// USED ANALOG PINS (A0, A1, A2, A3, A4, A5, A6, A7, B0, B1)

// (BLACKPILL_F401 : FREE) : C13, C14, C15, B2. B12
//########################################################################
// FIXED HARDWARE MAPPING FOR STM32F4xx Series MCU's #########END#######
//########################################################################
#include <stdint.h>

#define FD_NONE 0
//================================================
// UART1 CONFIGURATION
//================================================
//#define FD_UART1_ENABLED
#define FD_UART1_RX_PIN        6
#define FD_UART1_RX_PORT       FD_NONE
#define FD_UART1_TX_PIN        7
#define FD_UART1_TX_PORT       FD_NONE
#define FD_UART1_TX_BUFFER_SIZE 256
#define FD_UART1_RX_BUFFER_SIZE 256



//================================================
// SIL SIMULATION CONFIGURATION
//================================================
#define SIL_DEFAULT_VCC 36.0f
#define SIL_MAX_TIMESTEP (1.0/20000.0)
// Number of SIL steps to run for each PWM update 
//(for better resolution in the scope plots, better simulation quality and for marginally low time constants)
#define SIL_HALL_OFFSET 0
static const uint8_t SIL_HALL_TABLE_PI_3[6] = {
    ((1 << 2) | (1 << 1) | (0 << 0)), // 110
    ((0 << 2) | (1 << 1) | (0 << 0)), // 010
    ((0 << 2) | (1 << 1) | (1 << 0)), // 011
    ((0 << 2) | (0 << 1) | (1 << 0)), // 001
    ((1 << 2) | (0 << 1) | (1 << 0)), // 101
    ((1 << 2) | (0 << 1) | (0 << 0))  // 100
};

#endif//fd_driver_conf.h