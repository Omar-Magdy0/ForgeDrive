/**
 * @file    fd_usbcdc.h
 * @author  Carol Nasser
 * @brief   USB CDC (Communication Device Class) Driver.
 * @details Implements a Virtual COM Port (VCP) interface for the STM32F4, 
 * using the UART handle structure for consistent data streaming.
 */

#pragma once

#include "fd_uart.h"
#include "fd_driver_conf.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initializes the USB CDC peripheral.
 * @param  handle: UART handle instance to manage USB streams.
 */
void fd_usbcdc_init(fd_uart_handle_t *handle);

/**
 * @brief  Sends data over USB CDC.
 * @param  handle: USB CDC handle.
 * @param  data: Pointer to data buffer.
 * @param  len: Length of data to send.
 * @retval uint8_t: Status or bytes written.
 */
uint8_t fd_usbcdc_write(fd_uart_handle_t *handle, uint8_t* data, uint8_t len);

/**
 * @brief  Receives data from USB CDC.
 * @param  handle: USB CDC handle.
 * @param  data: Pointer to buffer for incoming data.
 * @param  len: Maximum bytes to read.
 * @retval uint16_t: Actual bytes read.
 */
uint16_t fd_usbcdc_read(fd_uart_handle_t *handle, uint8_t* data, uint8_t len);

/** @brief Resets all USB CDC traffic statistics. */
void fd_usbcdc_resetStats(fd_uart_handle_t *handle);

#ifdef __cplusplus
}
#endif