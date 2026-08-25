#ifndef USBXCH
#define USBXCH
#include <stdint.h>
#include <stdbool.h>
#include "TCPServer.h"


typedef struct{
    TCPServer tcps;
}fd_usbxch_handle_t;

void fd_usbxch_init(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_write(fd_usbxch_handle_t *h, const uint8_t* data, uint32_t length);
uint32_t fd_usbxch_write_available(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_flush(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_read(fd_usbxch_handle_t *h, uint8_t *data, uint32_t length);
uint32_t fd_usbxch_read_available(fd_usbxch_handle_t *h);
bool fd_usbxch_connected(fd_usbxch_handle_t *h);
void fd_usbxch_update(fd_usbxch_handle_t *h);

#endif


