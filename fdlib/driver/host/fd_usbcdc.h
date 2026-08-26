#pragma once

#include "fd_uart.h"
#include "fd_driver_conf.h"


#ifdef __cplusplus
extern "C" {
#endif



void fd_usbcdc_init(fd_uart_handle_t *handle);
uint8_t fd_usbcdc_write(fd_uart_handle_t *handle, uint8_t* data, uint8_t len);
uint16_t fd_usbcdc_read(fd_uart_handle_t *handle, uint8_t* data, uint8_t len);
void fd_usbcdc_resetStats(fd_uart_handle_t *handle);

#ifdef __cplusplus
}
#endif
