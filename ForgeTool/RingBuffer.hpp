#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <vector>

namespace ecolib
{

class RingBuffer
{
public:
    explicit RingBuffer(size_t capacity = 16384)
    {
        // Align capacity to the next power of two for fast bitwise operations
        capacity_ = 1;
        while (capacity_ < capacity)
        {
            capacity_ <<= 1;
        }
        mask_ = capacity_ - 1;
        buf_.resize(capacity_);
    }

    size_t capacity() const { return capacity_; }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return head_ == tail_;
    }

    size_t available() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return tail_ - head_;
    }

    size_t push(const uint8_t *data, size_t len)
    {
        if (!data || len == 0)
            return 0;

        std::lock_guard<std::mutex> lock(mutex_);
        const size_t free_space = capacity_ - (tail_ - head_);
        const size_t to_write = std::min(len, free_space);

        const size_t write_pos = tail_ & mask_;
        const size_t first_chunk = std::min(to_write, capacity_ - write_pos);

        std::memcpy(&buf_[write_pos], data, first_chunk);
        if (to_write > first_chunk)
        {
            std::memcpy(&buf_[0], data + first_chunk, to_write - first_chunk);
        }

        tail_ += to_write;
        return to_write;
    }

    size_t pop(uint8_t *dst, size_t len)
    {
        if (!dst || len == 0)
            return 0;

        std::lock_guard<std::mutex> lock(mutex_);
        const size_t used_space = tail_ - head_;
        const size_t to_read = std::min(len, used_space);

        const size_t read_pos = head_ & mask_;
        const size_t first_chunk = std::min(to_read, capacity_ - read_pos);

        std::memcpy(dst, &buf_[read_pos], first_chunk);
        if (to_read > first_chunk)
        {
            std::memcpy(dst + first_chunk, &buf_[0], to_read - first_chunk);
        }

        head_ += to_read;
        return to_read;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = 0;
        tail_ = 0;
    }

private:
    std::vector<uint8_t> buf_;
    size_t capacity_{0};
    size_t mask_{0};
    size_t head_{0};
    size_t tail_{0};
    mutable std::mutex mutex_;
};

} // namespace ecolib

// Global alias for backward compatibility
using RingBuffer = ecolib::RingBuffer;