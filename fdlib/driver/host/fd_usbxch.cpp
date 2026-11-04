#include "fd_usbxch.h"
#include "fd_SILServer.h"
#include <stdio.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector.h>
#define SILSERVER_PORT_USBXCH 5001

{% set inst = usbxch %}
fd_usbxch_handle_t {{inst.name}};

std::vector<uint8_t> rxring_mock1;
std::vector<uint8_t> rxring_mock2;

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

uint32_t fd_usbxch_{{inst.name}}_write(fd_usbxch_handle_t *h, const uint8_t *data, uint32_t length)
{
    SILServer &server = SILServer::instance();
    size_t written = 0;
    server.write(SILSERVER_PORT_USBXCH, data, length, written);
    return written;
}

uint32_t fd_usbxch_{{inst.name}}_read(fd_usbxch_handle_t *h, uint8_t *data, uint32_t length)
{
    SILServer &server = SILServer::instance();
    asio::error_code ec;
    return server.read(SILSERVER_PORT_USBXCH, data, length, ec);
}

bool fd_usbxch_{{inst.name}}_connected(fd_usbxch_handle_t *h)
{
    SILServer &server = SILServer::instance();
    return server.connected(SILSERVER_PORT_USBXCH);
}

void fd_usbxch_{{inst.name}}_update(fd_usbxch_handle_t *h)
{
  (void)h;
}

uint32_t fd_usbxch_{{inst.name}}_write_available(fd_usbxch_handle_t *h)
{
    (void)h;
    return UINT32_MAX;
}
uint32_t fd_usbxch_{{inst.name}}_flush(fd_usbxch_handle_t *h)
{
    (void)h;
    return 0;
}
uint32_t fd_usbxch_{{inst.name}}_read_available(fd_usbxch_handle_t *h)
{
    SILServer &server = SILServer::instance();
    return server.read_available(SILSERVER_PORT_USBXCH);
}

uint32_t fd_usbxch_{{inst.name}}_clear(fd_usbxch_handle_t *h)
{
    (void)h;
    return 0;
}

bool fd_usbxch_{{inst.name}}_read_reserve(fd_usbxch_handle_t *h,
                                         void** readptr1,
                                         uint16_t* cont1,
                                         void** readptr2,
                                         uint16_t* cont2)
{
    (void)h;
    size_t available = server.read_available(SILSERVER_PORT_USBXCH);
    if (!available)
        return false;
    std::vector<uint8_t> incoming(available);
    asio::error_code ec;
    size_t received = server.read(
        SILSERVER_PORT_USBXCH, incoming.data(), available, ec);
    if (ec || !received)
        return false;
    auto split1 = std::rand() % (received + 1);
    auto split2 = received - split1;
    rxring_mock1.assign(incoming.begin(), incoming.begin() + split1);
    rxring_mock2.assign(incoming.begin() + split1, incoming.begin() + received);
    
    *readptr1 = rxring_mock1.data();
    *readptr2 = rxring_mock2.data();
    *cont1 = static_cast<uint16_t>(split1);
    *cont2 = static_cast<uint16_t>(split2);
    return true;
}

bool fd_usbxch_{{inst.name}}_read_commit(fd_usbxch_handle_t *h, size_t len)
{
    (void)h;

    size_t total = rxring_mock1.size() + rxring_mock2.size();

    if (len > total)
        return false;

    size_t consume1 = std::min(len, rxring_mock1.size());
    size_t consume2 = len - consume1;

    rxring_mock1.erase(rxring_mock1.begin(),
                       rxring_mock1.begin() + consume1);

    rxring_mock2.erase(rxring_mock2.begin(),
                       rxring_mock2.begin() + consume2);

    return true;
}