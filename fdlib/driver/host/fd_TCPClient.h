#pragma once

#include <asio.hpp>
#include <string>
#include <cstdint>
#include <iostream>

using asio::ip::tcp;

class TCPClient
{
private:
    asio::io_context io_context_;
    tcp::socket socket_;
    bool connected_ = false;


public:
    TCPClient() : socket_(io_context_) {}

    ~TCPClient() {
        close();
    }

    bool connect(const std::string& host, uint16_t port, size_t rx_buf_kb = 1024, size_t tx_buf_kb = 1024) {
        asio::error_code ec;
        tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host, std::to_string(port), ec);
        if (ec) {
            std::cerr << "[TCPClient] Resolve error: " << ec.message() << "\n";
            return false;
        }

        asio::connect(socket_, endpoints, ec);
        if (ec) {
            std::cerr << "[TCPClient] Connection failed to port " << port << ": " << ec.message() << "\n";
            return false;
        }

        socket_.set_option(asio::socket_base::send_buffer_size(tx_buf_kb * 1024), ec);
        socket_.set_option(asio::socket_base::receive_buffer_size(rx_buf_kb * 1024), ec);
            
        // Set socket to non-blocking mode to mirror server architecture
        socket_.non_blocking(true, ec);
        if (ec) {
            std::cerr << "[TCPClient] Failed to set non-blocking mode: " << ec.message() << "\n";
            socket_.close();
            return false;
        }

        connected_ = true;
        std::cout << "[TCPClient] Successfully connected to " << host << ":" << port << "\n";
        return true;
    }

    // Explicit close API
    void close() {
        asio::error_code ec;
        if (socket_.is_open()) {
            socket_.close(ec);
        }
        if (connected_) {
            connected_ = false;
            std::cout << "[TCPClient] Connection closed.\n";
        }
    }

    bool is_connected() const {
        return connected_ && socket_.is_open();
    }

    // Non-blocking write
    bool write(const uint8_t* data, size_t len, size_t& written) {
        if (!is_connected()) return false;

        asio::error_code ec;
        written = socket_.write_some(asio::buffer(data, len), ec);

        if (ec == asio::error::would_block) {
            written = 0;
            return true; // Buffer full right now, try again later
        } 
        else if (ec) {
            std::cerr << "[TCPClient] Write error: " << ec.message() << "\n";
            close();
            return false;
        }
        return true;
    }

    // Non-blocking read poll
    size_t read(uint8_t* buffer, size_t max_len, asio::error_code& ec) {
        if (!is_connected()) {
            ec = asio::error::not_connected;
            return 0;
        }

        size_t n = socket_.read_some(asio::buffer(buffer, max_len), ec);

        if (ec == asio::error::would_block) {
            ec.clear(); // Normal for non-blocking when no data is waiting
            return 0;
        } 
        else if (ec) {
            std::cerr << "[TCPClient] Read error: " << ec.message() << "\n";
            close();
        }
        return n;
    }

    // Check bytes available to read immediately
    size_t read_available() {
        if (!is_connected()) return 0;
        asio::error_code ec;
        return socket_.available(ec);
    }
};