#pragma once

#include <asio.hpp>
#include <unordered_map>
#include <memory>
#include <thread>
#include <iostream>
#include <cstdint>

using asio::ip::tcp;

class SILServer
{

private:
    struct SILPort
    {
        uint16_t port;
        tcp::acceptor acceptor;
        tcp::socket socket;
        bool connected = false;

        SILPort(asio::io_context& io, uint16_t p) 
            : port(p), 
              acceptor(io, tcp::endpoint(tcp::v4(), p)), 
              socket(io) {}
    };

    asio::io_context io_context_;
    std::thread io_thread_;
    bool running_ = false;
    std::unordered_map<uint16_t, std::unique_ptr<SILPort>> ports_;

    SILServer() : io_context_(1) {
        running_ = true;
        io_thread_ = std::thread([this]() {
            io_context_.run();
        });
    }

public:
    static SILServer& instance() {
        static SILServer inst;
        return inst;
    }

    ~SILServer() {
        running_ = false;
        io_context_.stop();
        if (io_thread_.joinable()) io_thread_.join();
    }

    // Register a port for a peripheral and start its background accept loop
    bool register_port(uint16_t port) {
        if (ports_.find(port) != ports_.end()) return false;

        auto channel = std::make_unique<SILPort>(io_context_, port);

        // Launch background accept coroutine for this specific port
        asio::co_spawn(io_context_, [this, port]() -> asio::awaitable<void> {
            auto& chan = ports_[port];
            for (;;) {
                asio::error_code ec;
                tcp::socket new_sock = co_await chan->acceptor.async_accept(
                    asio::redirect_error(asio::use_awaitable, ec)
                );
                if (!ec) {
                    if (chan->socket.is_open()) chan->socket.close();
                    chan->socket = std::move(new_sock);
                    chan->socket.non_blocking(true, ec);
                    chan->connected = !ec;
                    std::cout << "[SIL Server] Client connected to port " << port << "\n";
                }
            }
        }(), asio::detached);

        ports_[port] = std::move(channel);
        return true;
    }

    // Non-blocking write
    bool write(uint16_t port, const uint8_t* data, size_t len, size_t& written) {
        auto it = ports_.find(port);
        if (it == ports_.end() || !it->second->connected) return false;
        asio::error_code ec;
        written = it->second->socket.write_some(asio::buffer(data, len), ec);
        if (ec == asio::error::would_block) {
            written = 0;
            return true;
        } else if (ec) {
            it->second->connected = false;
            it->second->socket.close();
            return false;
        }
        return true;
    }

    // Non-blocking read poll
    size_t read(uint16_t port, uint8_t* buffer, size_t max_len, asio::error_code& ec) {
        auto it = ports_.find(port);
        if (it == ports_.end() || !it->second->connected) {
            ec = asio::error::not_connected;
            return 0;
        }

        size_t n = it->second->socket.read_some(asio::buffer(buffer, max_len), ec);
        if (ec == asio::error::would_block) {
            ec.clear();
            return 0;
        } else if (ec) {
            it->second->connected = false;
            it->second->socket.close();
        }
        return n;
    }

    size_t read_available(uint16_t port) {
        auto it = ports_.find(port);
        if (it == ports_.end() || !it->second->connected) return 0;

        asio::error_code ec;
        return it->second->socket.available(ec); 
    }

    // Check if a client is currently connected to a specific peripheral port
    bool connected(uint16_t port) const {
        auto it = ports_.find(port);
        if (it == ports_.end() || !it->second) {
            return false;
        }
        return it->second->connected;
    }
};