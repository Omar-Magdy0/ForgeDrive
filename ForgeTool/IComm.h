#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>
class IComm
{
public:
    virtual ~IComm() = default;

    // Connection management
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // Data transfer
    virtual bool write(std::span<const uint8_t> data) = 0;
    virtual size_t read(std::span<uint8_t> data) = 0;
    bool write(const std::vector<uint8_t>& data)
    {
        return write(std::span<const uint8_t>{data});
    }
    size_t read(std::vector<uint8_t>& data)
    {
        return read(std::span<uint8_t>{data});
    }
    virtual size_t available() = 0;

    // Events
    std::function<void()> onConnect;
    std::function<void()> onDisconnect;
    std::function<void(int)> onError;
};