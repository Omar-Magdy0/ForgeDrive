#pragma once

#include <libusb-1.0/libusb.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "fd/ring.h"

struct UsbDeviceInfo
{
    uint16_t vid;
    uint16_t pid;

    uint8_t bus;
    uint8_t address;

    std::string manufacturer;
    std::string product;
    std::string serial_number;
    std::vector<uint8_t> interfaces;
};

class Usbxch
{
public:
    static constexpr size_t RXBUF_SIZE = (1 << 24);
    static constexpr size_t TXBUF_SIZE = (1 << 24);

    Usbxch()
        : rxbuf_(new uint8_t[RXBUF_SIZE]),
          rx_stream(rxbuf_.get(), RXBUF_SIZE),
          txbuf_(new uint8_t[TXBUF_SIZE]),
          tx_stream(txbuf_.get(), TXBUF_SIZE)
    {
    }

    ~Usbxch();

    static std::vector<UsbDeviceInfo> scan();
    bool connect(const std::string& serial_number,
                 uint16_t vid,
                 uint16_t pid,
                 int interface_number,
                 unsigned char bulk_in_ep,
                 unsigned char bulk_out_ep,
                 int timeout_ms);

    void disconnect();
    bool isConnected() const;
    void (*onConnect)(void *ctx) = nullptr;
    void (*onDisconnect)(void *ctx) = nullptr;
    void (*onError)(void *ctx, int ec) = nullptr;
    void *callback_ctx = nullptr;
    inline static int timeout = 5000;

    int write(const uint8_t* data, size_t length);
    int write(const std::vector<uint8_t>& data)
    {
        return write(data.data(), data.size());
    }
    int read(uint8_t* data, size_t max_len);
    size_t available();

private:
    libusb_context* context_ = nullptr;
    std::thread event_thread_;
    std::atomic<bool> event_running_{false};
    void event_loop();
    int arm_rx();
    int arm_tx();
    void finish_disconnect();
    libusb_device_handle *handle_ = nullptr;
    libusb_transfer* rx_transfer = nullptr;
    libusb_transfer* tx_transfer = nullptr;

    std::atomic<bool> disconnect_req{false};
    std::atomic<bool> connected_{false};
    std::atomic<bool> rx_active_{false};
    std::atomic<bool> tx_active_{false};

    std::unique_ptr<uint8_t[]> rxbuf_;
    fd::ring_fast<uint8_t> rx_stream;
    std::atomic<bool> rx_paused_{false};

    std::unique_ptr<uint8_t[]> txbuf_;
    fd::ring_fast<uint8_t> tx_stream;

    int interface_number_ = 0;

    unsigned char bulk_in_ep_ = 0;
    unsigned char bulk_out_ep_ = 0;

    static void LIBUSB_CALL tx_callback(libusb_transfer* transfer);
    static void LIBUSB_CALL rx_callback(libusb_transfer* transfer);
};
