#pragma once
#include <stdint.h>
#include <string.h>
#include <type_traits>
#include <etl/span.h>

namespace vraw
{

enum class Err : uint8_t
{
    OK = 0,
    BUFFER_FULL,
    BUFFER_EMPTY,
};

class Writer
{
    etl::span<uint8_t> buf;
    uint16_t buf_idx = 0;

    Writer() = delete;

public:
    Writer(etl::span<uint8_t> buffer_) : buf(buffer_) {}

    template<typename T>
    Err add(const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);

        if (size_t(buf_idx) + sizeof(T) > buf.size())
            return Err::BUFFER_FULL;

        memcpy(buf.data() + buf_idx, &value, sizeof(T));
        buf_idx += sizeof(T);

        return Err::OK;
    }

    template<typename T>
    Err add(const T* data, uint16_t count)
    {
        static_assert(std::is_trivially_copyable_v<T>);

        size_t len = sizeof(T) * count;

        if (size_t(buf_idx) + len > buf.size())
            return Err::BUFFER_FULL;

        memcpy(buf.data() + buf_idx, data, len);
        buf_idx += len;

        return Err::OK;
    }

    Err add_BINARY(const uint8_t* data, uint16_t length)
    {
        if (size_t(buf_idx) + length > buf.size())
            return Err::BUFFER_FULL;

        memcpy(buf.data() + buf_idx, data, length);
        buf_idx += length;

        return Err::OK;
    }

    Err add_UINT8(uint8_t v)   { return add(v); }
    Err add_INT8(int8_t v)     { return add(v); }
    Err add_UINT16(uint16_t v) { return add(v); }
    Err add_INT16(int16_t v)   { return add(v); }
    Err add_UINT32(uint32_t v) { return add(v); }
    Err add_INT32(int32_t v)   { return add(v); }
    Err add_UINT64(uint64_t v) { return add(v); }
    Err add_INT64(int64_t v)   { return add(v); }
    Err add_FLOAT(float v)     { return add(v); }
    Err add_DOUBLE(double v)   { return add(v); }

    template<typename T>
    static constexpr size_t SERIALIZED_SIZE() { return sizeof(T); }
    static constexpr size_t BINARY_SIZE(size_t length) { return length; }

    uint16_t size() const { return buf_idx; }
    uint8_t* data() const { return buf.data(); }
    void reset() { buf_idx = 0; }
};

class Reader
{
    etl::span<const uint8_t> buf;
    uint16_t buf_idx = 0;

    Reader() = delete;

public:
    Reader(etl::span<const uint8_t> buffer_) : buf(buffer_) {}

    template<typename T>
    Err get(T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);

        if (size_t(buf_idx) + sizeof(T) > buf.size())
            return Err::BUFFER_EMPTY;

        memcpy(&value, buf.data() + buf_idx, sizeof(T));
        buf_idx += sizeof(T);

        return Err::OK;
    }

    template<typename T>
    Err get(T* data, uint16_t count)
    {
        static_assert(std::is_trivially_copyable_v<T>);

        size_t len = sizeof(T) * count;

        if (size_t(buf_idx) + len > buf.size())
            return Err::BUFFER_EMPTY;

        memcpy(data, buf.data() + buf_idx, len);
        buf_idx += len;

        return Err::OK;
    }

    Err get_BINARY(uint8_t* data, uint16_t length)
    {
        if (size_t(buf_idx) + length > buf.size())
            return Err::BUFFER_EMPTY;

        memcpy(data, buf.data() + buf_idx, length);
        buf_idx += length;

        return Err::OK;
    }

    uint16_t remaining() const { return buf.size() - buf_idx; }
    uint16_t position() const { return buf_idx; }

    void reset() { buf_idx = 0; }
};

}
