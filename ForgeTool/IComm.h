#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>
class IComm
{
public:
    virtual ~IComm() = default;

    // Data transfer
    virtual bool write(std::span<const uint8_t> data) = 0;
    virtual size_t read(std::span<uint8_t> data) = 0;
    virtual size_t available() = 0;

    // Events
    std::function<void()> onConnect;
    std::function<void()> onDisconnect;
    std::function<void(int)> onError;
};