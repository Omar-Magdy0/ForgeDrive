#pragma once
#include "fd/fd_ring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t flow_control;
}fd_uart_config_t;

typedef struct{
    uint8_t tx_busy;
    uint8_t rx_busy;
    uint8_t rx_overflow;
    uint8_t tx_overflow;
}fd_uart_status_t;

typedef struct{
    fd_uart_config_t config;
    fd_uart_status_t status;
    int fd;
}fd_uart_handle_t;

void fd_uart1_init(fd_uart_handle_t *handle);
uint8_t fd_uart1_write(fd_uart_handle_t *handle, const uint8_t* data, uint8_t len);
uint16_t fd_uart1_read(fd_uart_handle_t *handle, uint8_t* data, uint8_t len);


#ifdef __cplusplus
}
#endif