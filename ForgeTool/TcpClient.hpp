#pragma once

#include <asio.hpp>
#include <cstdint>
#include <string>

class TcpClient
{
public:
    TcpClient() 
        : socket_(io_context_) 
    {}

    ~TcpClient()
    {

    }

    bool connect(const std::string& host, uint16_t port)
    {

    }

    void disconnect()
    {

    }

    bool is_connected() const
    {

    }

    int available() const
    {

    }

    int write(const void* data, size_t length)
    {

    }

    int read(void* buffer, size_t length)
    {

    }

private:
    asio::io_context io_context_;
    asio::ip::tcp::socket socket_;
};