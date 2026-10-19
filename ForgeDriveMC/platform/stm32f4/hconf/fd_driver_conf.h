/**
 * @file    fd_driver_conf.h
 * @author  Carol Nasser
 * @brief   Global Configuration for STM32F4 eldrivers.
 * @details Defines hardware mappings, buffer sizes, and peripheral enable 
 * flags for the entire ForgeDriveMC driver suite.
 */

#ifndef DRV_CONF_H
#define DRV_CONF_H
#include "stm32f4xx_ll_adc.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_tim.h"
#include "stm32f4xx_ll_rcc.h"

/** @name Hardware Mapping Reference
 * Fixed mappings for STM32F4xx series (UART, SWD, USB, CAN, Timers).
 * @{ */
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
/** @} */

//=============================
// MC2P
#define FD_MC2P_BG_CHANNELS 0

/** @name Motor Control (MC3P) Configuration */
//================================================
// MCADCPWM3P CONFIGURATION    
//================================================


/** @name Hall Sensor Configuration */
//================================================
// HALL SENSOR CONFIGURATION    
//================================================
#define FD_HALL1_ENABLED            
#define FD_HALL1_A_PIN              LL_GPIO_PIN_15
#define FD_HALL1_A_PORT             GPIOA
#define FD_HALL1_B_PIN              LL_GPIO_PIN_3
#define FD_HALL1_B_PORT             GPIOB
#define FD_HALL1_C_PIN              LL_GPIO_PIN_10
#define FD_HALL1_C_PORT             GPIOB





#endif//fd_driver_conf.h