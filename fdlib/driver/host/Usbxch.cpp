#include "Usbxch.h"
#include "SILServer.h"
#include <stdio.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#define SILSERVER_PORT_USBXCH 5001

{% set inst = usbxch %}
FdUsbxchHandle {{inst.name}};

std::vector<uint8_t> rxring_mock1;
std::vector<uint8_t> rxring_mock2;

void FdUsbxch_{{inst.name}}_Init(FdUsbxchHandle *h)
{
    SILServer &server = SILServer::Instance();
    auto suc = server.RegisterPort(SILSERVER_PORT_USBXCH);
    if(!suc)
    {
        std::cerr << "[SIL Server] USBXCH PORT init fail" << std::endl;
    }else
    {
        std::cout << "[SIL Server] USBXCH PORT INITIALIZED" << std::endl;
    }
}

uint32_t FdUsbxch_{{inst.name}}_Write(FdUsbxchHandle *h, const uint8_t *data, uint32_t length)
{
    SILServer &server = SILServer::Instance();
    size_t written = 0;
    server.Write(SILSERVER_PORT_USBXCH, data, length, written);
    return written;
}

uint32_t FdUsbxch_{{inst.name}}_Read(FdUsbxchHandle *h, uint8_t *data, uint32_t length)
{
    SILServer &server = SILServer::Instance();
    asio::error_code ec;
    return server.Read(SILSERVER_PORT_USBXCH, data, length, ec);
}

bool FdUsbxch_{{inst.name}}_Connected(FdUsbxchHandle *h)
{
    SILServer &server = SILServer::Instance();
    return server.Connected(SILSERVER_PORT_USBXCH);
}

void FdUsbxch_{{inst.name}}_Update(FdUsbxchHandle *h)
{
  (void)h;
}

uint32_t FdUsbxch_{{inst.name}}_WriteAvailable(FdUsbxchHandle *h)
{
    (void)h;
    return UINT32_MAX;
}
uint32_t FdUsbxch_{{inst.name}}_Flush(FdUsbxchHandle *h)
{
    (void)h;
    return 0;
}
uint32_t FdUsbxch_{{inst.name}}_ReadAvailable(FdUsbxchHandle *h)
{
    SILServer &server = SILServer::Instance();
    return server.ReadAvailable(SILSERVER_PORT_USBXCH);
}

uint32_t FdUsbxch_{{inst.name}}_Clear(FdUsbxchHandle *h)
{
    (void)h;
    return 0;
}

bool FdUsbxch_{{inst.name}}_ReadReserve(FdUsbxchHandle *h,
                                         void** readptr1,
                                         uint16_t* cont1,
                                         void** readptr2,
                                         uint16_t* cont2)
{
    (void)h;
    SILServer &server = SILServer::Instance();
    size_t available = server.ReadAvailable(SILSERVER_PORT_USBXCH);
    if (!available)
        return false;
    std::vector<uint8_t> incoming(available);
    asio::error_code ec;
    size_t received = server.Read(SILSERVER_PORT_USBXCH, incoming.data(), available, ec);
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

bool FdUsbxch_{{inst.name}}_ReadCommit(FdUsbxchHandle *h, size_t len)
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