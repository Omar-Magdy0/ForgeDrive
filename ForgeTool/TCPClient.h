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
    static inline asio::io_context io_context_;
    static inline std::thread io_thread;
    tcp::socket socket_;
    std::string host_;
    uint16_t port_;
    std::deque<std::vector<uint8_t>> tx_queue_;
    bool writing_ = false;
    static inline std::vector<TCPClient*> connections = {};
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
    TCPClient():socket_(io_context_)
    {
    }
    ~TCPClient() override
    {
        disconnect();
    }
    void disconnect()
    {
        asio::error_code ec;
        if (socket_.is_open())
            socket_.close(ec);
        auto it = std::find(connections.begin(), connections.end(), this);
        if(it != connections.end())
            connections.erase(it);
        if(onDisconnect)onDisconnect();
    }
public:
    static bool init()
    {
        io_context_.restart();
        io_thread = std::thread([](){io_context_.run();});
        return true;
    }
    static bool deinit()
    {
        io_context_.stop();
        if (io_thread.joinable())
            io_thread.join();
        return true;
    }
    static TCPClient* connect(const std::string &host, uint16_t port, std::string &error, size_t rx_buf_kb = 1024, size_t tx_buf_kb = 1024)
    {
        auto existing = findConnection(host, port); 
        if(existing)return existing;
        asio::error_code ec;
        auto client = new TCPClient;
        tcp::resolver resolver(io_context_);
        do
        {
            auto endpoints = resolver.resolve(host, std::to_string(port), ec);
            if(ec)break;
            asio::connect(client->socket_, endpoints, ec);
            if(ec)break;        
            client->socket_.set_option(asio::socket_base::send_buffer_size(tx_buf_kb * 1024), ec);
            client->socket_.set_option(asio::socket_base::receive_buffer_size(rx_buf_kb * 1024), ec);
        }while(0);
        if (ec)
        {
            error = ec.message();
            disconnect(client);
            return nullptr;
        }
        //if we reach here , all went great
        client->host_ = host;
        client->port_ = port;
        connections.push_back(client);
        return client;
    }

    void getAddress(std::string &host, uint16_t &port)
    {
        host = host_;
        port = port_;
    }
    
    static TCPClient* findConnection(const std::string& host, uint16_t port)
    {
        auto it = std::find_if(connections.begin(), connections.end(),
            [&](const TCPClient* client)
            {
                return client->host_ == host && client->port_ == port;
            });

        return it != connections.end() ? *it : nullptr;
    }

    static void disconnect(TCPClient* client)
    {
        client->disconnect();
        delete client;
    }

    bool write(std::span<const uint8_t> data) override
    {
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
        size_t n = 0;
        if (socket_.available() > 0)
            n = socket_.read_some(asio::buffer(data.data(), data.size()));
        return n;
    }

    // Check bytes available to read immediately
    size_t available() override
    {
        asio::error_code ec;
        return socket_.available(ec);
    }

    static const std::vector<TCPClient*>& getConnections()
    {
        return connections;
    }
};