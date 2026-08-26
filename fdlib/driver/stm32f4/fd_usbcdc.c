#include "fd_usbcdc.h"

#ifdef FD_USBCDC_ENABLED
void fd_usbcdc_init(fd_uart_handle_t *handle)
{
}

uint8_t fd_usbcdc_write(fd_uart_handle_t *handle, uint8_t* data, uint8_t len)
{    
}

uint16_t fd_usbcdc_read(fd_uart_handle_t *handle, uint8_t* data, uint8_t len)
{
    return 0;
}

fd_ring_stats_t fd_usbcdc_rx_stats(fd_uart_handle_t *handle)
{
}

fd_ring_stats_t fd_usbcdc_tx_stats(fd_uart_handle_t *handle)
{
}

void fd_usbcdc_resetStats(fd_uart_handle_t *handle)
{
}

#endif