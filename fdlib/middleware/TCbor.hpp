#pragma once
#include "cbor.h"
#include <stdint.h>
#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "etl/span.h"
#include "etl/vector.h"
#include "etl/string.h"
#include "etl/tuple.h"

#define MAX_NESTING 5
enum class TCborType : uint8_t
{
    Integer,
    ByteString,
    TextString,
    Array,
    Map,
    Tag,
    Simple,
    Boolean,
    Null,
    Undefined,
    HalfFloat,
    Float,
    Double,
    Invalid = 0xff
};

struct TCborParse
{
    CborParser parser;
    CborValue it[MAX_NESTING + 1];
    TCborType container_type[MAX_NESTING];
    size_t container_length[MAX_NESTING];
    uint8_t depth = 0;
    bool valid = false;
    TCborParse(etl::span<const uint8_t> rx_buf)
    {
        valid = cbor_parser_init(rx_buf.data(), rx_buf.size(), CborValidateBasic, &parser, &it[0]) == CborNoError;
        depth = 0;
    }
    bool isArray(const CborValue &value) const
    {
        return cbor_value_is_array(&value);
    }
    bool isArray() const
    {
        return isArray(it[depth]);
    }
    bool isMap(const CborValue &value) const
    {
        return cbor_value_is_map(&value);
    }
    bool isMap() const
    {
        return isMap(it[depth]);
    }
    bool isString(const CborValue &value) const
    {
        return cbor_value_is_text_string(&value);
    }
    bool isString() const
    {
        return isString(it[depth]);
    }
    bool isBytes(const CborValue &value) const
    {
        return cbor_value_is_byte_string(&value);
    }
    bool isBytes() const
    {
        return isBytes(it[depth]);
    }
    bool isInteger(const CborValue &value) const
    {
        return cbor_value_is_integer(&value);
    }
    bool isInteger() const
    {
        return isInteger(it[depth]);
    }
    bool isFloat(const CborValue &value) const
    {
        return cbor_value_is_float(&value);
    }
    bool isFloat() const
    {
        return isFloat(it[depth]);
    }
    bool isBoolean(const CborValue &value) const
    {
        return cbor_value_is_boolean(&value);
    }
    bool isBoolean() const
    {
        return isBoolean(it[depth]);
    }
    bool isNull(const CborValue &value) const
    {
        return cbor_value_is_null(&value);
    }
    bool isNull() const
    {
        return isNull(it[depth]);
    }
    template <typename T>
    bool getINT(CborValue &value, T &v)
    {
        static_assert(std::is_integral_v<T>, "Integer type required");
        if (cbor_value_at_end(&value))
            return false;
        if constexpr (std::is_unsigned_v<T>)
        {
            if (!cbor_value_is_unsigned_integer(&value))
                return false;
            uint64_t raw_value;
            if (cbor_value_get_uint64(&value, &raw_value) != CborNoError)
                return false;
            if (raw_value > std::numeric_limits<T>::max())
                return false;
            v = static_cast<T>(raw_value);
        }
        else
        {
            if (!cbor_value_is_integer(&value))
                return false;
            int64_t raw_value;
            if (cbor_value_get_int64(&value, &raw_value) != CborNoError)
                return false;
            if (raw_value < std::numeric_limits<T>::min() ||
                raw_value > std::numeric_limits<T>::max())
                return false;
            v = static_cast<T>(raw_value);
        }
        cbor_value_advance_fixed(&value);
        return true;
    }
    template <typename T>
    bool getINT(T &v)
    {
        return getINT(it[depth], v);
    }
    template <typename T>
    bool getFLOAT(CborValue &value, T &v)
    {
        static_assert(std::is_floating_point_v<T>,
                      "Floating-point type required");
        if (cbor_value_at_end(&value))
            return false;
        if constexpr (std::is_same_v<T, float>)
        {
            if (!cbor_value_is_float(&value))
                return false;
            if (cbor_value_get_float(&value, &v) != CborNoError)
                return false;
        }
        else
        {
            if (!cbor_value_is_double(&value))
                return false;
            if (cbor_value_get_double(&value, &v) != CborNoError)
                return false;
        }
        cbor_value_advance_fixed(&value);
        return true;
    }
    template <typename T>
    bool getFLOAT(T &v)
    {
        return getFLOAT(it[depth], v);
    }
    bool getBOOL(CborValue &value, bool &v)
    {
        if (cbor_value_at_end(&value))
            return false;
        if (!cbor_value_is_boolean(&value))
            return false;
        if (cbor_value_get_boolean(&value, &v) != CborNoError)
            return false;
        cbor_value_advance_fixed(&value);
        return true;
    }
    bool getBOOL(bool &v)
    {
        return getBOOL(it[depth], v);
    }
    bool getSTRING(CborValue &value, etl::istring &out)
    {
        if (cbor_value_at_end(&value))
            return false;
        if (!cbor_value_is_text_string(&value))
            return false;

        size_t size = out.capacity();
        if (cbor_value_copy_text_string(
                &value, out.data(), &size, &value) != CborNoError)
            return false;
        if(size >= out.capacity())return false; //null termination didn't succeed;
        out.resize(size);
        return true;
    }
    bool getSTRING(etl::istring &out)
    {
        return get(it[depth], out);
    }
    bool getBYTES(CborValue &value, etl::ivector<uint8_t> &out)
    {
        if (cbor_value_at_end(&value))
            return false;
        if (!cbor_value_is_byte_string(&value))
            return false;
        size_t size = out.capacity();
        if (cbor_value_copy_byte_string(&value, out.data(), &size, &value) != CborNoError)
            return false;
        out.resize(size);
        return true;
    }
    bool getBYTES(etl::ivector<uint8_t> &out)
    {
        return getBYTES(it[depth], out);
    }
    bool getNull(CborValue &value)
    {
        if (cbor_value_at_end(&value))
            return false;
        if (!cbor_value_is_null(&value))
            return false;
        cbor_value_advance_fixed(&value);
        return true;
    }
    bool getNull()
    {
        return getNull(it[depth]);
    }
    template <typename Tuple>
    bool getTUPLE(CborValue &value, Tuple &v)
    {
        if (!enterArray())return false;
        if (!etl::apply([this, &value](auto &...values)
                        { return (get(value, values) && ...); }, v))
            return false;
        if (!leaveArray())return false;
        return true;
    }
    template <typename T>
    bool get(CborValue &value, T &v)
    {
        using U = std::remove_cv_t<T>;
        if constexpr (etl::is_tuple<U>::value)
        {
            return getTUPLE(value, v);
        }
        else if constexpr (std::is_same_v<U, bool>)
        {
            return getBOOL(value, v);
        }
        else if constexpr (std::is_enum_v<U>)
        {
            using B = std::underlying_type_t<U>;
            B raw{};
            if (!get(value, raw)) return false;
            v = static_cast<U>(raw);
            return true;
        }
        else if constexpr (std::is_integral_v<U>)
        {
            return getINT(value, v);
        }
        else if constexpr (std::is_floating_point_v<U>)
        {
            return getFLOAT(value, v);
        }
        else if constexpr (std::is_base_of_v<etl::istring, U>)
        {
            return getSTRING(value, v);
        }
        else if constexpr (std::is_base_of_v<etl::ivector<uint8_t>, U>)
        {
            return getBYTES(value, v);
        }
        else
        {
            static_assert(!sizeof(T), "Unsupported CBOR destination type");
        }
    }
    template <typename T>
    bool get(T &v)
    {
        return get(it[depth], v);
    }
    TCborType getType(CborValue &value)
    {
        switch (cbor_value_get_type(&value))
        {
        case CborIntegerType:
            return TCborType::Integer;
        case CborByteStringType:
            return TCborType::ByteString;
        case CborTextStringType:
            return TCborType::TextString;
        case CborArrayType:
            return TCborType::Array;
        case CborMapType:
            return TCborType::Map;
        case CborTagType:
            return TCborType::Tag;
        case CborSimpleType:
            return TCborType::Simple;
        case CborBooleanType:
            return TCborType::Boolean;
        case CborNullType:
            return TCborType::Null;
        case CborUndefinedType:
            return TCborType::Undefined;
        case CborHalfFloatType:
            return TCborType::HalfFloat;
        case CborFloatType:
            return TCborType::Float;
        case CborDoubleType:
            return TCborType::Double;
        case CborInvalidType:
            return TCborType::Invalid;
        }

        return TCborType::Invalid;
    }
    TCborType getType() { return getType(it[depth]); }
    bool hasNext() const
    {
        return !cbor_value_at_end(it + depth);
    }
    bool atEnd() const
    {
        return cbor_value_at_end(it + depth);
    }
    bool skip()
    {
        if (!valid || atEnd())
            return false;

        return cbor_value_advance(&it[depth]) == CborNoError;
    }
    bool enterContainer(TCborType type)
    {
        if (!valid || depth >= MAX_NESTING)
            return false;

        CborValue &parent = it[depth];
        size_t len;

        if (type == TCborType::Array)
        {
            if (!cbor_value_is_array(&parent))
                return false;

            if (cbor_value_get_array_length(&parent, &len) != CborNoError)
                return false;
        }
        else
        {
            if (!cbor_value_is_map(&parent))
                return false;

            if (cbor_value_get_map_length(&parent, &len) != CborNoError)
                return false;
        }

        if (cbor_value_enter_container(&parent, &it[depth + 1]) != CborNoError)
            return false;

        container_type[depth] = type;
        container_length[depth] = len;
        ++depth;

        return true;
    }
    bool enterArray()
    {
        return enterContainer(TCborType::Array);
    }
    bool enterMap()
    {
        return enterContainer(TCborType::Map);
    }
    bool leaveContainer()
    {
        if (!valid || depth == 0)
            return false;

        if (cbor_value_leave_container(
                &it[depth - 1], &it[depth]) != CborNoError)
            return false;

        --depth;
        return true;
    }
    bool leaveArray()
    {
        return depth > 0 &&
               container_type[depth - 1] == TCborType::Array &&
               leaveContainer();
    }
    bool leaveMap()
    {
        return depth > 0 &&
               container_type[depth - 1] == TCborType::Map &&
               leaveContainer();
    }
    bool containerLength(size_t &length) const
    {
        if (!depth)
            return false;

        length = container_length[depth - 1];
        return true;
    }
    template <typename T>
    bool key(T &v)
    {
        return get(v);
    }
};

struct TCborEncode
{
    CborEncoder it[MAX_NESTING + 1];
    uint8_t depth = 0;
    uint8_t *_tx_buf_ptr = NULL;
    TCborEncode(etl::span<uint8_t> tx_buf)
    {
        cbor_encoder_init(&it[0], tx_buf.data(), tx_buf.size(), CborValidateBasic);
        _tx_buf_ptr = tx_buf.data();
        depth = 0;
    }
    template <typename T>
    bool putINT(const T &v)
    {
        static_assert(std::is_integral_v<T>, "Integer type required");
        if constexpr (std::is_unsigned_v<T>)
            return cbor_encode_uint(&it[depth], static_cast<uint64_t>(v)) == CborNoError;
        else
            return cbor_encode_int(&it[depth], static_cast<int64_t>(v)) == CborNoError;
    }
    template <typename T>
    bool putFLOAT(const T &v)
    {
        static_assert(std::is_floating_point_v<T>, "Floating-point type required");

        if constexpr (std::is_same_v<T, float>)
            return cbor_encode_float(&it[depth], v) == CborNoError;
        else
            return cbor_encode_double(&it[depth], v) == CborNoError;
    }
    bool putBOOL(const bool &v)
    {
        return cbor_encode_boolean(&it[depth], v) == CborNoError;
    }
    bool putSTRING(const etl::istring &v)
    {
        return cbor_encode_text_string(&it[depth], v.data(), v.size()) == CborNoError;
    }
    bool putBYTES(const etl::ivector<uint8_t>& v)
    {
        return cbor_encode_byte_string(
            &it[depth], v.data(), v.size()) == CborNoError;
    }
    bool putNull()
    {
        return cbor_encode_null(&it[depth]) == CborNoError;
    }
    bool enterArray(size_t length)
    {
        if (depth >= MAX_NESTING)
            return false;
        if (cbor_encoder_create_array(&it[depth], &it[depth + 1], length) != CborNoError)
            return false;
        ++depth;
        return true;
    }
    bool enterMap(size_t pairs)
    {
        if (depth >= MAX_NESTING)
            return false;
        if (cbor_encoder_create_map(&it[depth], &it[depth + 1], pairs) != CborNoError)
            return false;
        ++depth;
        return true;
    }
    template <typename K>
    bool enterMap(const K &key, size_t pairs)
    {
        return put(key) && enterMap(pairs);
    }
    template <typename K>
    bool enterArray(const K &key, size_t length)
    {
        return put(key) && enterArray(length);
    }
    bool leaveContainer()
    {
        if (depth == 0)
            return false;

        if (cbor_encoder_close_container(&it[depth - 1], &it[depth]) != CborNoError)
            return false;

        --depth;
        return true;
    }
    bool leaveArray() { return leaveContainer(); }
    bool leaveMap()   { return leaveContainer(); }
    template <typename Tuple>
    bool putTUPLE(const Tuple& v)
    {
        if (!enterArray(etl::tuple_size<Tuple>::value))
            return false;
        if (!etl::apply([this](const auto &...values)
                        { return (put(values) && ...); }, v))
            return false;
        if (!leaveArray())
            return false;
        return true;
    }
    template <typename K, typename T>
    bool put(const K &key, const T &v)
    {
        return put(key) && put(v);
    }
    template <typename T>
    bool put(const T &v)
    {
        using U = std::remove_cv_t<T>;
        if constexpr (etl::is_tuple<U>::value)
            return putTUPLE(v);
        else if constexpr (std::is_same_v<U, bool>)
            return putBOOL(v);
        else if constexpr (std::is_base_of_v<etl::istring, U>)
            return putSTRING(v);
        else if constexpr (std::is_base_of_v<etl::ivector<uint8_t>, U>)
            return putBYTES(v);
        else if constexpr (std::is_enum_v<U>)
            return put(static_cast<std::underlying_type_t<U>>(v));
        else if constexpr (std::is_floating_point_v<U>)
            return putFLOAT(v);
        else if constexpr (std::is_integral_v<U>)
            return putINT(v);
        else
            static_assert(!sizeof(U), "Unsupported CBOR type");
    }
    size_t size()
    {
        return cbor_encoder_get_buffer_size(&it[0], _tx_buf_ptr);
    }
};