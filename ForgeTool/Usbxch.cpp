#include "Usbxch.h"

#include <algorithm>

namespace
{
std::string read_string_descriptor(libusb_device_handle* handle, uint8_t index)
{
    if (index == 0)
        return {};

    unsigned char buffer[256];
    const int length = libusb_get_string_descriptor_ascii(
        handle,
        index,
        buffer,
        sizeof(buffer));

    if (length <= 0)
        return {};

    return {reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(length)};
}
}

Usbxch::~Usbxch()
{
    disconnect();
    if (event_thread_.joinable())
        event_thread_.join();
}

int Usbxch::arm_rx()
{
    if (!connected_.load())
        return false;

    const size_t available =
        rx_stream.capacity() - rx_stream.size();

    if (available == 0)
        return false;

    bool expected = false;
    
    if (!rx_active_.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acquire))
    {
        return false;
    }

    size_t c1, c2;
    uint8_t *w1, *w2;

    rx_stream.write_reserve(
        available,
        &w1, &c1,
        &w2, &c2
    );

    libusb_fill_bulk_transfer(
        rx_transfer,
        handle_,
        bulk_in_ep_,
        w1,
        c1,
        rx_callback,
        this,
        0
    );

    const int rc = libusb_submit_transfer(rx_transfer);

    if (rc != LIBUSB_SUCCESS)
    {
        rx_active_.store(false, std::memory_order_release);
        if(onError)onError(callback_ctx, rc);
        return false;
    }    
    return true;
}

int Usbxch::arm_tx()
{
    if (!connected_.load())
        return false;

    bool expected = false;
    if (!tx_active_.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acquire))
    {
        return false;
    }

    const size_t to_send = tx_stream.size();

    if (to_send == 0)
        return false;

    size_t c1, c2;
    uint8_t *r1, *r2;

    tx_stream.read_reserve(
        &r1, &c1,
        &r2, &c2
    );

    libusb_fill_bulk_transfer(
        tx_transfer,
        handle_,
        bulk_out_ep_,
        r1,
        c1,
        tx_callback,
        this,
        0
    );

    const int rc = libusb_submit_transfer(tx_transfer);

    if (rc != LIBUSB_SUCCESS)
    {
        tx_active_.store(false, std::memory_order_release);
        onError(callback_ctx, rc);
        return false;
    }    

    return true;
}

void Usbxch::finish_disconnect()
{
    libusb_free_transfer(rx_transfer);
    rx_transfer = nullptr;

    libusb_free_transfer(tx_transfer);
    tx_transfer = nullptr;

    libusb_release_interface(handle_, interface_number_);
    libusb_close(handle_);
    handle_ = nullptr;

    libusb_exit(context_);
    context_ = nullptr;

    connected_ = false;

    if (onDisconnect)
        onDisconnect(callback_ctx);

    event_running_ = false;
    disconnect_req.store(false);
}

std::vector<UsbDeviceInfo> Usbxch::scan()
{
    libusb_context* context = nullptr;
    if (libusb_init(&context) != LIBUSB_SUCCESS)
        return {};

    libusb_device** devices = nullptr;
    const ssize_t count = libusb_get_device_list(context, &devices);
    if (count < 0)
    {
        libusb_exit(context);
        return {};
    }

    std::vector<UsbDeviceInfo> result;
    result.reserve(static_cast<std::size_t>(count));

    for (ssize_t i = 0; i < count; ++i)
    {
        libusb_device* device = devices[i];
        libusb_device_descriptor descriptor{};
        if (libusb_get_device_descriptor(device, &descriptor) != LIBUSB_SUCCESS)
            continue;

        UsbDeviceInfo info{};
        info.vid = descriptor.idVendor;
        info.pid = descriptor.idProduct;
        info.bus = libusb_get_bus_number(device);
        info.address = libusb_get_device_address(device);

        libusb_config_descriptor* configuration = nullptr;
        if (libusb_get_active_config_descriptor(device, &configuration) == LIBUSB_SUCCESS)
        {
            for (uint8_t interface_index = 0;
                 interface_index < configuration->bNumInterfaces;
                 ++interface_index)
            {
                const libusb_interface& interface = configuration->interface[interface_index];
                if (interface.num_altsetting == 0)
                    continue;

                const uint8_t number = interface.altsetting[0].bInterfaceNumber;
                if (std::find(info.interfaces.begin(), info.interfaces.end(), number) == info.interfaces.end())
                    info.interfaces.push_back(number);
            }
            libusb_free_config_descriptor(configuration);
        }

        libusb_device_handle* handle = nullptr;
        if (libusb_open(device, &handle) == LIBUSB_SUCCESS)
        {
            info.manufacturer = read_string_descriptor(handle, descriptor.iManufacturer);
            info.product = read_string_descriptor(handle, descriptor.iProduct);
            info.serial_number = read_string_descriptor(handle, descriptor.iSerialNumber);
            libusb_close(handle);
        }

        result.push_back(std::move(info));
    }

    libusb_free_device_list(devices, 1);
    libusb_exit(context);
    return result;
}

bool Usbxch::connect(const std::string &serial_number, uint16_t vid, uint16_t pid, int interface_number, unsigned char bulk_in_ep, unsigned char bulk_out_ep, int timeout_ms)
{
    if (interface_number < 0 || timeout_ms < 0)
        return false;

    disconnect();
    if (event_thread_.joinable())
        event_thread_.join();
    disconnect_req.store(false);

    if (libusb_init(&context_) != LIBUSB_SUCCESS)
    {
        context_ = nullptr;
        return false;
    }

    libusb_device** devices = nullptr;
    const ssize_t count = libusb_get_device_list(context_, &devices);
    if (count < 0)
    {
        libusb_exit(context_);
        context_ = nullptr;
        return false;
    }

    bool claimed = false;
    for (ssize_t i = 0; i < count && !claimed; ++i)
    {
        libusb_device_descriptor descriptor{};
        if (libusb_get_device_descriptor(devices[i], &descriptor) != LIBUSB_SUCCESS ||
            descriptor.idVendor != vid ||
            descriptor.idProduct != pid)
        {
            continue;
        }

        libusb_device_handle* candidate = nullptr;
        if (libusb_open(devices[i], &candidate) != LIBUSB_SUCCESS)
            continue;

        if (!serial_number.empty() &&
            read_string_descriptor(candidate, descriptor.iSerialNumber) != serial_number)
        {
            libusb_close(candidate);
            continue;
        }

        // This is unsupported on some platforms. claim_interface() below is
        // authoritative: it will fail if a kernel driver still owns it.
        libusb_set_auto_detach_kernel_driver(candidate, 1);

        if (libusb_claim_interface(candidate, interface_number) != LIBUSB_SUCCESS)
        {
            libusb_close(candidate);
            continue;
        }

        handle_ = candidate;
        claimed = true;
    }

    libusb_free_device_list(devices, 1);

    if (!claimed)
    {
        libusb_exit(context_);
        context_ = nullptr;
        return false;
    }

    interface_number_ = interface_number;
    bulk_in_ep_ = bulk_in_ep;
    bulk_out_ep_ = bulk_out_ep;
    connected_.store(true, std::memory_order_release);

    //clean the streams
    rx_stream.clear(); 
    tx_stream.clear();

    //arm rx
    arm_rx();

    event_running_.store(true, std::memory_order_release);
    event_thread_ = std::thread(USB1::event_loop, this);
    if(onConnect)onConnect(callback_ctx);
    return true;
}

bool Usbxch::isConnected() const
{
    return connected_.load(std::memory_order_acquire);
}

void Usbxch::disconnect()
{
    connected_.store(false, std::memory_order_release);
    if (rx_transfer)
        libusb_cancel_transfer(rx_transfer);
    if (tx_transfer)
        libusb_cancel_transfer(tx_transfer);
    disconnect_req.store(true);
}

int Usbxch::write(const uint8_t *data, size_t length)
{
    //Arm using the asynchronous libusb API
    size_t c1, c2;
    uint8_t *w1, *w2;
    bool suc = tx_stream.write_reserve(length, &w1, &c1, &w2, &c2);
    if(!suc)return false;
    memcpy(w1, data, c1);
    if(c2) memcpy(w2, data+c1, c2);
    tx_stream.write_commit(length);

    arm_tx();
    return suc;
}

int Usbxch::read(uint8_t* data, size_t max_len)
{
    auto c = rx_stream.size();

    if (!c || max_len == 0)
        return 0;

    auto rd = std::min(c, max_len);

    size_t c1, c2;
    uint8_t* r1;
    uint8_t* r2;

    rx_stream.read_reserve(&r1, &c1, &r2, &c2);

    size_t remaining = rd;

    size_t n1 = std::min<size_t>(c1, remaining);
    memcpy(data, r1, n1);
    remaining -= n1;

    if (remaining)
    {
        size_t n2 = std::min<size_t>(c2, remaining);
        memcpy(data + n1, r2, n2);
    }

    rx_stream.read_commit(rd);
    if(rx_paused_.load(std::memory_order_acquire))
    {
        rx_paused_.store(false, std::memory_order_release);
        //re-arm
        arm_rx();
    }
    return static_cast<int>(rd);
}

size_t Usbxch::available()
{
    return rx_stream.size();
}

void Usbxch::event_loop()
{
    while (event_running_.load(std::memory_order_acquire))
    {
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = timeout;
        int rc = libusb_handle_events_timeout(context_, &tv);
        if (rc != LIBUSB_SUCCESS)
        {
            if(onError)onError(callback_ctx, rc);
        }
        //also finish events/requests here ,, like the disconnect one 
        if (disconnect_req.load())
        {
            if(!rx_active_.load() && !tx_active_.load()){finish_disconnect();}
        }
    }
}

void LIBUSB_CALL Usbxch::rx_callback(libusb_transfer* transfer)
{
    auto ctx = (Usbxch*)transfer->user_data;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        // Ring buffer/memory pointer update and commit memory
        // Check for the written ammount 
        auto len = transfer->actual_length;
        ctx->rx_stream.write_commit(len);
        ctx->rx_active_.store(false);
        auto empty_space = ctx->rx_stream.capacity() - ctx->rx_stream.size();
        if (empty_space == 0) // if empty space is zero, pause and return early, we will rearm in read/consumption
        {
            ctx->rx_paused_.store(true, std::memory_order_release);

            return;
        }
        ctx->arm_rx();
    } else if (transfer->status == LIBUSB_TRANSFER_CANCELLED || transfer->status == LIBUSB_TRANSFER_NO_DEVICE)
    {
        ctx->rx_active_.store(false);
    } else if (transfer->status == LIBUSB_TRANSFER_ERROR)
    {
        if(ctx->onError)ctx->onError(ctx->callback_ctx, transfer->status );
        ctx->rx_active_.store(false);
    }
}

void LIBUSB_CALL Usbxch::tx_callback(libusb_transfer* transfer)
{
    auto ctx = (Usbxch*)transfer->user_data;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        // Ring buffer/memory pointer update and commit memory
        ctx->tx_active_.store(false);
        auto len = transfer->actual_length;
        ctx->tx_stream.read_commit(len);
        ctx->arm_tx();
    } else if (transfer->status == LIBUSB_TRANSFER_CANCELLED || transfer->status == LIBUSB_TRANSFER_NO_DEVICE)
    {
        ctx->tx_active_.store(false);
    } else if (transfer->status == LIBUSB_TRANSFER_ERROR)
    {
        if(ctx->onError)ctx->onError(ctx->callback_ctx, transfer->status);
        ctx->tx_active_.store(false);
    }
}
