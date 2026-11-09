#pragma once

#include <asio.hpp>
#include <string>
#include <cstdint>
#include <thread>
#include <iostream>
#include "IComm.h"

using asio::ip::tcp;
class TCPClient : public IComm
{
private:
    asio::io_context io_context_;
    std::thread io_thread;
    tcp::socket socket_;
    std::deque<std::vector<uint8_t>> tx_queue_;
    bool writing_ = false;

    void startWrite()
    {
        if (tx_queue_.empty() || !socket_.is_open())
            return;

        writing_ = true;

        asio::async_write(
            socket_,
            asio::buffer(tx_queue_.front()),
            [this](const asio::error_code &ec, std::size_t bytes)
            {
                writing_ = false;

                if (ec)
                {
                    // Handle disconnection / transmission error
                    tx_queue_.clear();
                    return;
                }
                tx_queue_.pop_front();
                startWrite();
            });
    }

public:
    TCPClient() : socket_(io_context_) {}

    ~TCPClient() override
    {
        disconnect();
        if (io_thread.joinable())
            io_thread.join();
    }

    bool connect(const std::string &host, uint16_t port, size_t rx_buf_kb = 1024, size_t tx_buf_kb = 1024)
    {

        if (io_thread.joinable())
            io_thread.join();
        io_context_.restart();
        asio::error_code ec;
        tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host, std::to_string(port), ec);
        if (ec)
        {
            std::cerr << "[TCPClient] Resolve error: " << ec.message() << "\n";
            return false;
        }
        asio::connect(socket_, endpoints, ec);
        if (ec)
        {
            onError(ec.value());
            return false;
        }
        socket_.set_option(asio::socket_base::send_buffer_size(tx_buf_kb * 1024), ec);
        socket_.set_option(asio::socket_base::receive_buffer_size(rx_buf_kb * 1024), ec);
        if (ec)
        {
            onError(ec.value());
            return false;
        }
        io_thread = std::thread([this]()
                                { io_context_.run(); });
        return true;
    }

    // Explicit close API
    void disconnect() override
    {
        asio::error_code ec;
        if (socket_.is_open())
            socket_.close(ec);
        onDisconnect();
    }

    bool isConnected() const override
    {
        return socket_.is_open();
    }

    bool write(std::span<const uint8_t> data) override
    {
        if (!isConnected())
            return false;
        if(data.empty())return true;
        std::vector<uint8_t> buffer(data.begin(), data.end());
        asio::post(io_context_, [this, buffer = std::move(buffer)]() mutable
                   {
            tx_queue_.push_back(std::move(buffer));

            if (!writing_)
                startWrite(); });

        return true;
    }

    // Non-blocking read poll
    size_t read(std::span<uint8_t> data) override
    {
        if (!isConnected())
        {
            return 0;
        }
        size_t n = 0;
        if (socket_.available() > 0)
            n = socket_.read_some(asio::buffer(data.data(), data.size()));
        return n;
    }

    // Check bytes available to read immediately
    size_t available() override
    {
        if (!isConnected())
            return 0;
        asio::error_code ec;
        return socket_.available(ec);
    }
};