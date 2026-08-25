#include "fd_usbxch.h"
#include <stdio.h>
#include <iostream>


void fd_usbxch_init(fd_usbxch_handle_t *h)
{
    bool success = h->tcps.init(4001);
    if(!success)
    {
        std::cerr << "USBXCH TCP init fail" << std::endl;
    }else
    {
        std::cout << "USBXCH TCP INITIALIZED" << std::endl;
    }
}

uint32_t fd_usbxch_write(fd_usbxch_handle_t *h, const uint8_t *data, uint32_t length)
{
    return h->tcps.write(data, length);
}

uint32_t fd_usbxch_read(fd_usbxch_handle_t *h, uint8_t *data, uint32_t length)
{
    return h->tcps.read(data, length);
}

bool fd_usbxch_connected(fd_usbxch_handle_t *h)
{
    return h->tcps.isConnected();
}

void fd_usbxch_update(fd_usbxch_handle_t *h)
{
    h->tcps.pollClient();
}

uint32_t fd_usbxch_write_available(fd_usbxch_handle_t *h)
{
    return UINT16_MAX;
}
uint32_t fd_usbxch_flush(fd_usbxch_handle_t *h)
{
    return 0;
}
uint32_t fd_usbxch_read_available(fd_usbxch_handle_t *h)
{
    return h->tcps.available();
}