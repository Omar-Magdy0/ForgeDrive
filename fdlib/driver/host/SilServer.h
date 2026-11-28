#pragma once

#include <asio.hpp>
#include <unordered_map>
#include <memory>
#include <thread>
#include <algorithm>
#include <iostream>
#include <cstdint>

using asio::ip::tcp;
#ifndef SILSERVER_IP_ADDRESS
#define SILSERVER_IP_ADDRESS "127.0.0.1"
#endif
enum SilServerPort
{
    SilServerPort_Usbxch = 5001
};


class SilServer
{

private:
    struct SilPort
    {
        uint16_t port;
        tcp::acceptor acceptor;
        tcp::socket socket;
        bool connected = false;
        SilPort(asio::io_context &io, uint16_t p, const char *address)
            : port(p),
              acceptor(io, tcp::endpoint(asio::ip::make_address(address), p)),
              socket(io) {}
    };
    asio::io_context io_context_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_{asio::make_work_guard(io_context_)};
    std::thread io_thread_;
    bool running_ = false;
    std::unordered_map<uint16_t, std::unique_ptr<SilPort>> ports_;

    SilServer() : io_context_(1)
    {
        running_ = true;
        io_thread_ = std::thread([this]()
                                 { io_context_.run(); });
    }

public:
    static SilServer &Instance()
    {
        static SilServer inst;
        return inst;
    }

    ~SilServer()
    {
        running_ = false;
        io_context_.stop();
        if (io_thread_.joinable())
            io_thread_.join();
    }

    // Register a port for a peripheral and start its background accept loop
    bool RegisterPort(uint16_t port, size_t rx_buf_kb = 1024, size_t tx_buf_kb = 1024)
    {
        if (ports_.find(port) != ports_.end())
            return false;
        auto channel = std::make_unique<SilPort>(io_context_, port, SILSERVER_IP_ADDRESS);
        auto *chan = channel.get();
        ports_[port] = std::move(channel);
        asio::co_spawn(io_context_, [chan, port, rx_buf_kb, tx_buf_kb]() -> asio::awaitable<void>
                       {
        for (;;) {
            asio::error_code ec;
            tcp::socket new_sock = co_await chan->acceptor.async_accept(
                asio::redirect_error(asio::use_awaitable, ec)
            );
            if (ec) {
                if (ec == asio::error::operation_aborted)
                    co_return;

                continue;
            }
            new_sock.set_option(asio::socket_base::send_buffer_size(tx_buf_kb * 1024), ec);
            new_sock.set_option(asio::socket_base::receive_buffer_size(rx_buf_kb * 1024), ec);
            if (chan->socket.is_open())
                chan->socket.close();
            chan->socket = std::move(new_sock);
            chan->connected = true;
            std::cout << "[SIL Server] Client connected to port " << port << "\n";
        } }, asio::detached);

        return true;
    }

    bool Write(uint16_t port, const uint8_t *data, size_t len, size_t &written)
    {
        auto it = ports_.find(port);
        if (it == ports_.end())
            return false;
        asio::error_code ec;
        written = it->second->socket.write_some(asio::buffer(data, len), ec);
        return !ec;
    }

    size_t Read(uint16_t port, uint8_t *buffer, size_t max_len, asio::error_code &ec)
    {
        auto it = ports_.find(port);
        if (it == ports_.end())
        {
            ec = asio::error::not_connected;
            return 0;
        }
        auto &socket = it->second->socket;
        size_t available = socket.available(ec);
        if (ec || available == 0)
            return 0;
        size_t n = socket.read_some(asio::buffer(buffer, std::min(available, max_len)), ec);
        return ec ? 0 : n;
    }

    size_t ReadAvailable(uint16_t port)
    {
        auto it = ports_.find(port);
        if (it == ports_.end())
            return 0;

        asio::error_code ec;
        size_t n = it->second->socket.available(ec);
        return ec ? 0 : n;
    }

    // Check if a client is currently connected to a specific peripheral port
    bool Connected(uint16_t port) const
    {
        auto it = ports_.find(port);
        if (it == ports_.end() || !it->second)
        {
            return false;
        }
        return it->second->connected;
    }
};
