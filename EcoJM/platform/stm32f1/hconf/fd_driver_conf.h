/**
 * @file    fd_driver_conf.h
 * @author  Carol Nasser
 * @brief   Global Configuration for STM32F1 eldrivers.
 * @details Defines hardware mappings, buffer sizes, and peripheral enable 
 * flags for the entire ForgeDriveMC driver suite.
 */

#ifndef DRV_CONF_H
#define DRV_CONF_H
#include "stm32f1xx_ll_adc.h"
#include "stm32f1xx_ll_dma.h"
#include "stm32f1xx_ll_tim.h"
#include "stm32f1xx_ll_rcc.h"


/** @name UART1 Configuration */
//================================================
// UART1 CONFIGURATION
//================================================
//#define FD_UART1_ENABLED
#define FD_UART1_RX_PIN        6
#define FD_UART1_RX_PORT       GPIOB
#define FD_UART1_TX_PIN        7
#define FD_UART1_TX_PORT       GPIOB
#define FD_UART1_TX_BUFFER_SIZE 256
#define FD_UART1_RX_BUFFER_SIZE 256

/** @name USB CDC Configuration */
//================================================
// USBCDC CONFIGURATION
//================================================
//#define FD_USBCDC_ENABLED
#define FD_USBCDC_TX_BUFFER_SIZE 64
#define FD_USBCDC_RX_BUFFER_SIZE 64

//================================================
// USBXCH CONFIGURATION
//================================================
#define FD_USBXCH_ENABLED
#define FD_USBXCH_TX_BUFFSIZE 2048
#define FD_USBXCH_RX_BUFFSIZE 256


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/** @name DMA & NVIC Configuration */
//================================================
//DMA CONFIGURATION
//================================================
//NEVER FORGET 
//THAT YOU EVER CHANGE THESE VALUES FOR DMA YOU STILL HAVE TO CHANGE IMPLEMENTATION AND IRQ APPROPIATELY
#define UART1_DMA_TX_STREAM             LL_DMA_STREAM_7
#define UART1_DMA_RX_STREAM             LL_DMA_STREAM_5
#define UART1_DMA_INSTANCE              DMA2
#define UART1_DMA_CHANNEL               LL_DMA_CHANNEL_4

//================================================
// NVIC CONFIGURATION
//================================================
#define UART1_NVIC_PRIORITY 4


#endif//fd_driver_conf.h