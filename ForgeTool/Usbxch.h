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
#include <functional>
#include "IComm.h"


struct UsbInterfaceInfo
{
    uint8_t number;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    std::string name;
};

struct UsbDeviceInfo
{
    uint16_t vid;
    uint16_t pid;

    uint8_t bus;
    uint8_t address;

    std::string manufacturer;
    std::string product;
    std::string serial_number;
    std::vector<UsbInterfaceInfo> interfaces;
    libusb_device* _libusb_device;

    friend bool operator==(const UsbDeviceInfo& a, const UsbDeviceInfo& b)
    {
        return a.vid == b.vid &&
               a.pid == b.pid &&
               a.serial_number == b.serial_number;
    }
};

class Usbxch : public IComm
{
private:
    Usbxch()
        : rxbuf_(new uint8_t[RXBUF_SIZE]),
          rx_stream(rxbuf_.get(), RXBUF_SIZE),
          txbuf_(new uint8_t[TXBUF_SIZE]),
          tx_stream(txbuf_.get(), TXBUF_SIZE)
    {
    }
    ~Usbxch() override;
    void disconnect();
    
public:
    static constexpr uint32_t BULK_IN_EP = 0x81;
    static constexpr uint32_t BULK_OUT_EP = 0x01;
    static constexpr size_t RXBUF_SIZE = (1 << 24);
    static constexpr size_t TXBUF_SIZE = (1 << 24);

    static std::vector<UsbDeviceInfo>& scan();
    static bool init(unsigned int timeout_ms);
    static bool deinit();
    static Usbxch* connect(UsbDeviceInfo& device, std::string &error);
    static void disconnect(Usbxch* device);
    static const std::vector<Usbxch*>& getConnectedDevices();
    static const std::vector<UsbDeviceInfo>& getDiscoveredDevices();
    static bool isConnected(const UsbDeviceInfo& device);
    inline static int timeout = 100;
    const UsbDeviceInfo& getDeviceInfo() const;
    bool write(std::span<const uint8_t> data) override;
    size_t read(std::span<uint8_t> data) override;
    size_t available() override;

private:
    static inline libusb_context* context_ = nullptr;
    static inline std::mutex connected_devices_mutex_;
    static inline bool running = true;
    static inline std::vector<Usbxch*> connected_devices_ = {};
    static inline std::vector<UsbDeviceInfo> devices = {};
    static inline std::thread event_thread_;
    static inline void event_loop();
    static inline libusb_hotplug_callback_handle hotplug_handle;
    void finish_disconnect();
    int arm_rx();
    int arm_tx();
    UsbDeviceInfo device_info;
    libusb_device_handle *handle_ = nullptr;
    libusb_transfer* rx_transfer = nullptr;
    libusb_transfer* tx_transfer = nullptr;
    std::atomic<bool> disconnect_req{false};
    std::atomic<bool> rx_active_{false};
    std::atomic<bool> tx_active_{false};

    std::unique_ptr<uint8_t[]> rxbuf_;
    fd::ring_fast<uint8_t> rx_stream;
    std::atomic<bool> rx_paused_{false};

    std::unique_ptr<uint8_t[]> txbuf_;
    fd::ring_fast<uint8_t> tx_stream;
    int interface_number_ = 0;

    static void LIBUSB_CALL tx_callback(libusb_transfer* transfer);
    static void LIBUSB_CALL rx_callback(libusb_transfer* transfer);
    static int LIBUSB_CALL hotplugCallback(libusb_context* ctx, libusb_device* dev, libusb_hotplug_event event, void* user_data);
};
