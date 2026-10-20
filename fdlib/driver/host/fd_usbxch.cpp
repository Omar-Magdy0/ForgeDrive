#include "fd_usbxch.h"
#include "fd_SILServer.h"
#include <stdio.h>
#include <iostream>

#define SILSERVER_PORT_USBXCH 5001

{% for inst in usbxch.instances %}
fd_usbxch_handle_t {{inst.name}};
void fd_usbxch_{{inst.name}}_init(fd_usbxch_handle_t *h)
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
{% endfor %}

uint32_t fd_usbxch_write_impl(fd_usbxch_handle_t *h, const uint8_t *data, uint32_t length)
{
    SILServer &server = SILServer::instance();
    size_t written = 0;
    server.write(SILSERVER_PORT_USBXCH, data, length, written);
    return written;
}

uint32_t fd_usbxch_read_impl(fd_usbxch_handle_t *h, uint8_t *data, uint32_t length)
{
    SILServer &server = SILServer::instance();
    asio::error_code ec;
    return server.read(SILSERVER_PORT_USBXCH, data, length, ec);
}

bool fd_usbxch_connected_impl(fd_usbxch_handle_t *h)
{
    SILServer &server = SILServer::instance();
    return server.connected(SILSERVER_PORT_USBXCH);
}

void fd_usbxch_update_impl(fd_usbxch_handle_t *h)
{
  (void)h;
}

uint32_t fd_usbxch_write_available_impl(fd_usbxch_handle_t *h)
{
    (void)h;
    return UINT16_MAX;
}
uint32_t fd_usbxch_flush_impl(fd_usbxch_handle_t *h)
{
    (void)h;
    return 0;
}
uint32_t fd_usbxch_read_available_impl(fd_usbxch_handle_t *h)
{
    SILServer &server = SILServer::instance();
    return server.read_available(SILSERVER_PORT_USBXCH);
}