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
        disconnect();
    }

    bool connect(const std::string& host, uint16_t port)
    {
        disconnect();

        asio::error_code ec;
        asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host, std::to_string(port), ec);

        if (ec)
            return false;

        asio::connect(socket_, endpoints, ec);
        if (ec)
        {
            disconnect();
            return false;
        }

        // ضبط حجم الـ Buffers (4 MB)
        const int buffer_size = 4 * 1024 * 1024;
        socket_.set_option(asio::socket_base::send_buffer_size(buffer_size), ec);
        socket_.set_option(asio::socket_base::receive_buffer_size(buffer_size), ec);

        // جعل الـ Socket غير حجب (Non-blocking) كما في الكود الأصلي
        socket_.non_blocking(true, ec);

        return !ec;
    }

    void disconnect()
    {
        asio::error_code ec;
        if (socket_.is_open())
        {
            socket_.close(ec);
        }
    }

    bool is_connected() const
    {
        return socket_.is_open();
    }

    int available() const
    {
        if (!socket_.is_open())
            return -1;

        asio::error_code ec;
        std::size_t bytes = socket_.available(ec);
        return ec ? -1 : static_cast<int>(bytes);
    }

    int write(const void* data, size_t length)
    {
        if (!socket_.is_open())
            return -1;

        asio::error_code ec;
        size_t written = socket_.write_some(asio::buffer(data, length), ec);
        return ec ? -1 : static_cast<int>(written);
    }

    int read(void* buffer, size_t length)
    {
        if (!socket_.is_open())
            return -1;

        asio::error_code ec;
        size_t bytes_read = socket_.read_some(asio::buffer(buffer, length), ec);
        return ec ? -1 : static_cast<int>(bytes_read);
    }

private:
    asio::io_context io_context_;
    asio::ip::tcp::socket socket_;
};