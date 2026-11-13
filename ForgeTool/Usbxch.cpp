#include "Usbxch.h"
#include <algorithm>

namespace
{
    std::string read_string_descriptor(libusb_device_handle *handle, uint8_t index)
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

        return {reinterpret_cast<const char *>(buffer), static_cast<std::size_t>(length)};
    }
}

Usbxch::~Usbxch()
{
    disconnect();
}

int Usbxch::arm_rx()
{
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
        &w2, &c2);

    libusb_fill_bulk_transfer(
        rx_transfer,
        handle_,
        BULK_IN_EP,
        w1,
        c1,
        rx_callback,
        this,
        0);

    const int rc = libusb_submit_transfer(rx_transfer);

    if (rc != LIBUSB_SUCCESS)
    {
        rx_active_.store(false, std::memory_order_release);
        if (onError)
            onError(rc);
        return false;
    }
    return true;
}

int Usbxch::arm_tx()
{
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
        &r2, &c2);

    libusb_fill_bulk_transfer(
        tx_transfer,
        handle_,
        BULK_OUT_EP,
        r1,
        c1,
        tx_callback,
        this,
        0);

    const int rc = libusb_submit_transfer(tx_transfer);

    if (rc != LIBUSB_SUCCESS)
    {
        tx_active_.store(false, std::memory_order_release);
        if (onError)
            onError(rc);
        return false;
    }

    return true;
}

std::vector<UsbDeviceInfo> &Usbxch::scan()
{
    libusb_device **libusb_devices = nullptr;
    for (auto &device : devices)
    {
        if (device._libusb_device)
            libusb_unref_device(device._libusb_device);
    }
    devices.clear();
    const ssize_t count = libusb_get_device_list(context_, &libusb_devices);
    if (count < 0)
        return devices;
    for (ssize_t i = 0; i < count; ++i)
    {
        libusb_device *device = libusb_devices[i];
        libusb_device_descriptor descriptor{};
        if (libusb_get_device_descriptor(device, &descriptor) != LIBUSB_SUCCESS)
            continue;

        UsbDeviceInfo info{};
        info.vid = descriptor.idVendor;
        info.pid = descriptor.idProduct;
        info.bus = libusb_get_bus_number(device);
        info.address = libusb_get_device_address(device);
        info._libusb_device = libusb_ref_device(device);

        libusb_device_handle *handle = nullptr;
        bool opened = (libusb_open(device, &handle) == LIBUSB_SUCCESS);
        if (opened)
        {
            info.manufacturer = read_string_descriptor(handle, descriptor.iManufacturer);
            info.product = read_string_descriptor(handle, descriptor.iProduct);
            info.serial_number = read_string_descriptor(handle, descriptor.iSerialNumber);
        }
        libusb_config_descriptor *configuration = nullptr;
        if (libusb_get_active_config_descriptor(device, &configuration) == LIBUSB_SUCCESS)
        {
            for (uint8_t interface_index = 0; interface_index < configuration->bNumInterfaces; ++interface_index)
            {
                const libusb_interface &interface = configuration->interface[interface_index];
                if (interface.num_altsetting == 0)
                    continue;
                const auto &alt_setting = interface.altsetting[0];

                UsbInterfaceInfo intf_info{};
                intf_info.number = alt_setting.bInterfaceNumber;
                intf_info.bInterfaceClass = alt_setting.bInterfaceClass;
                intf_info.bInterfaceSubClass = alt_setting.bInterfaceSubClass;
                intf_info.bInterfaceProtocol = alt_setting.bInterfaceProtocol;
                if (opened && alt_setting.iInterface != 0) // Read optional human-readable interface name if handle is valid
                {
                    intf_info.name = read_string_descriptor(handle, alt_setting.iInterface);
                }
                // Avoid duplicate alternate settings for the same interface number
                auto it = std::find_if(info.interfaces.begin(), info.interfaces.end(),
                                       [&intf_info](const UsbInterfaceInfo &existing)
                                       {
                                           return existing.number == intf_info.number;
                                       });
                if (it == info.interfaces.end())
                {
                    info.interfaces.push_back(std::move(intf_info));
                }
            }
            libusb_free_config_descriptor(configuration);
        }
        if (opened)
        {
            libusb_close(handle);
        }
        devices.push_back(std::move(info));
    }
    libusb_free_device_list(libusb_devices, 0);
    return devices;
}

bool Usbxch::init(unsigned int timeout_ms)
{
    timeout = timeout_ms;
    running = true;
    if (context_ != nullptr)
        return true;
    int status = libusb_init(&context_);
    if (status != LIBUSB_SUCCESS)
    {
        context_ = nullptr;
        return false;
    }
    libusb_hotplug_register_callback(
        context_,
        LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
            LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
        LIBUSB_HOTPLUG_ENUMERATE,
        LIBUSB_HOTPLUG_MATCH_ANY,
        LIBUSB_HOTPLUG_MATCH_ANY,
        LIBUSB_HOTPLUG_MATCH_ANY,
        hotplugCallback,
        nullptr,
        &hotplug_handle);
    event_thread_ = std::thread(event_loop);
    libusb_set_option(context_, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING); // Configure libusb logging.
    scan();
    return true;
}

bool Usbxch::deinit()
{
    {
        std::lock_guard<std::mutex> lock(connected_devices_mutex_);
        for (auto &device : connected_devices_)
        {
            disconnect(device);
        }
    }
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(connected_devices_mutex_);
            if (connected_devices_.empty())
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    running = false;
    if (context_ == nullptr)
        return true;
    if (event_thread_.joinable())
        event_thread_.join();
    libusb_exit(context_);
    context_ = nullptr;
    return true;
}

Usbxch *Usbxch::connect(UsbDeviceInfo &device, std::string &error)
{
    if (device.interfaces.empty())
    {
        error = "NO_INTERFACES";
        return nullptr; 
    }
    libusb_device_handle *handle = nullptr;
    int interface_number = 0;
    int ec = LIBUSB_SUCCESS;
    do
    {
        // 1. Open device
        ec = libusb_open(device._libusb_device, &handle);
        if (ec != LIBUSB_SUCCESS)
            break;
        // 2. Enable automatic kernel driver detachment (where supported)
        ec = libusb_set_auto_detach_kernel_driver(handle, 1);
        if (ec != LIBUSB_SUCCESS)
            break;
        // 3. Claim interface
        interface_number = device.interfaces.at(0).number;
        ec = libusb_claim_interface(handle, interface_number);
        if (ec != LIBUSB_SUCCESS)
        {
            libusb_close(handle);
            break;
        }
    } while (0);
    if (ec != LIBUSB_SUCCESS)
    {
        error = libusb_error_name(ec);
        return nullptr;
    }

    // If we reached here , creation was a success
    Usbxch *ret = new Usbxch;
    ret->device_info = device;
    ret->handle_ = handle;
    ret->interface_number_ = interface_number;
    ret->rx_stream.clear();
    ret->tx_stream.clear();
    ret->disconnect_req.store(false);
    ret->tx_active_.store(false);
    ret->rx_active_.store(false);
    if (ret->onConnect)
        ret->onConnect();
    {
        std::lock_guard<std::mutex> lock(connected_devices_mutex_);
        connected_devices_.push_back(ret);
    }
    ret->arm_rx();
    return ret;
}

void Usbxch::disconnect(Usbxch *device)
{
    if (device)
    {
        device->disconnect();
    }
}

const UsbDeviceInfo &Usbxch::getDeviceInfo() const
{
    return device_info;
}

const std::vector<Usbxch *> &Usbxch::getConnectedDevices()
{
    return connected_devices_;
}

const std::vector<UsbDeviceInfo> &Usbxch::getDiscoveredDevices()
{
    return devices;
}

bool Usbxch::isConnected(const UsbDeviceInfo &device)
{
    const auto &connections = getConnectedDevices();

    return std::any_of(
        connections.begin(),
        connections.end(),
        [&](const Usbxch *conn)
        {
            return conn && conn->getDeviceInfo() == device;
        });
}

void Usbxch::disconnect()
{
    disconnect_req.store(true);
    if (rx_transfer)
        libusb_cancel_transfer(rx_transfer);
    if (tx_transfer)
        libusb_cancel_transfer(tx_transfer);
}

void Usbxch::finish_disconnect()
{
    if (rx_active_.load())
        libusb_free_transfer(rx_transfer);
    rx_transfer = nullptr;
    if (tx_active_.load())
        libusb_free_transfer(tx_transfer);
    tx_transfer = nullptr;

    libusb_release_interface(handle_, interface_number_);
    libusb_close(handle_);
    handle_ = nullptr;
    if (onDisconnect)
        onDisconnect();
}

bool Usbxch::write(std::span<const uint8_t> data)
{
    // Arm using the asynchronous libusb API
    size_t c1, c2;
    uint8_t *w1, *w2;
    bool suc = tx_stream.write_reserve(data.size(), &w1, &c1, &w2, &c2);
    if (!suc)
        return false;
    memcpy(w1, data.data(), c1);
    if (c2)
        memcpy(w2, data.data() + c1, c2);
    tx_stream.write_commit(data.size());
    arm_tx();
    return suc;
}

size_t Usbxch::read(std::span<uint8_t> data)
{
    auto c = rx_stream.size();
    if (!c || data.size() == 0)
        return 0;
    auto rd = std::min(c, data.size());
    size_t c1, c2;
    uint8_t *r1;
    uint8_t *r2;
    rx_stream.read_reserve(&r1, &c1, &r2, &c2);
    size_t remaining = rd;
    size_t n1 = std::min<size_t>(c1, remaining);
    memcpy(data.data(), r1, n1);
    remaining -= n1;
    if (remaining)
    {
        size_t n2 = std::min<size_t>(c2, remaining);
        memcpy(data.data() + n1, r2, n2);
    }
    rx_stream.read_commit(rd);
    if (rx_paused_.load(std::memory_order_acquire))
    {
        rx_paused_.store(false, std::memory_order_release);
        // re-arm
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
    while (running)
    {
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = timeout;
        libusb_handle_events_timeout(context_, &tv);
        std::vector<Usbxch *> disconnected;
        {
            std::lock_guard<std::mutex> lock(connected_devices_mutex_);
            for (auto it = connected_devices_.begin(); it != connected_devices_.end();)
            {
                Usbxch *dev = *it;
                if (dev->disconnect_req.load(std::memory_order_acquire) &&
                    !dev->rx_active_.load(std::memory_order_acquire) &&
                    !dev->tx_active_.load(std::memory_order_acquire))
                {
                    disconnected.push_back(dev);
                    it = connected_devices_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
        for (Usbxch *dev : disconnected)
        {
            dev->finish_disconnect();
            delete dev;
        }
    }
}

void LIBUSB_CALL Usbxch::rx_callback(libusb_transfer *transfer)
{
    auto ctx = (Usbxch *)transfer->user_data;
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
    }
    else if (transfer->status == LIBUSB_TRANSFER_CANCELLED || transfer->status == LIBUSB_TRANSFER_NO_DEVICE)
    {
        ctx->rx_active_.store(false);
    }
    else if (transfer->status == LIBUSB_TRANSFER_ERROR)
    {
        if (ctx->onError)
            ctx->onError(transfer->status);
        ctx->rx_active_.store(false);
    }
}

void LIBUSB_CALL Usbxch::tx_callback(libusb_transfer *transfer)
{
    auto ctx = (Usbxch *)transfer->user_data;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        // Ring buffer/memory pointer update and commit memory
        ctx->tx_active_.store(false);
        auto len = transfer->actual_length;
        ctx->tx_stream.read_commit(len);
        ctx->arm_tx();
    }
    else if (transfer->status == LIBUSB_TRANSFER_CANCELLED || transfer->status == LIBUSB_TRANSFER_NO_DEVICE)
    {
        ctx->tx_active_.store(false);
    }
    else if (transfer->status == LIBUSB_TRANSFER_ERROR)
    {
        if (ctx->onError)
            ctx->onError(transfer->status);
        ctx->tx_active_.store(false);
    }
}

// TODO: Finalize Hotplug feature and fully implement it
int LIBUSB_CALL Usbxch::hotplugCallback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
    {
    }
    else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)
    {
    }
    return 0;
}