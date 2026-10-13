#include "fd_usbxch.h"
#include <stdio.h>
#include <iostream>


void fd_usbxch_init(fd_usbxch_handle_t *h)
{
    SILServer &server = SILServer::instance();
    auto suc = server.register_port(SILSERVER_PORT_USBXCH);
    if(!suc)
    {
        std::cerr << "[SIL Server] USBXCH PORT init fail" << std::endl;
    }else
    {
        std::cout << "[SIL Server] USBXCH PORT INITIALIZED" << std::endl;
    }
}

uint32_t fd_usbxch_write(fd_usbxch_handle_t *h, const uint8_t *data, uint32_t length)
{
    SILServer &server = SILServer::instance();
    size_t written = 0;
    server.write(SILSERVER_PORT_USBXCH, data, length, written);
    return written;
}

uint32_t fd_usbxch_read(fd_usbxch_handle_t *h, uint8_t *data, uint32_t length)
{
    SILServer &server = SILServer::instance();
    asio::error_code ec;
    return server.read(SILSERVER_PORT_USBXCH, data, length, ec);
}

bool fd_usbxch_connected(fd_usbxch_handle_t *h)
{
    SILServer &server = SILServer::instance();
    return server.connected(SILSERVER_PORT_USBXCH);
}

void fd_usbxch_update(fd_usbxch_handle_t *h)
{
  (void)h;
}

uint32_t fd_usbxch_write_available(fd_usbxch_handle_t *h)
{
    (void)h;
    return UINT16_MAX;
}
uint32_t fd_usbxch_flush(fd_usbxch_handle_t *h)
{
    (void)h;
    return 0;
}
uint32_t fd_usbxch_read_available(fd_usbxch_handle_t *h)
{
    SILServer &server = SILServer::instance();
    return server.read_available(SILSERVER_PORT_USBXCH);
}