#pragma once
#include "cbor.h"
#include "etl/span.h"
#include "etl/string_view.h"
#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <limits>

#define MAX_NESTING 5
enum class CborType : uint8_t
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

struct CborParse
{
    CborParser parser;
    CborValue it[MAX_NESTING + 1];
    CborType container_type[MAX_NESTING];
    size_t container_length[MAX_NESTING];
    uint8_t depth = 0;
    bool valid = false;
    CborParse(etl::span<uint8_t> rx_buf)
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
        return valid && isArray(it[depth]);
    }
    bool isMap(const CborValue &value) const
    {
        return cbor_value_is_map(&value);
    }
    bool isMap() const
    {
        return valid && isMap(it[depth]);
    }
    bool isString(const CborValue &value) const
    {
        return cbor_value_is_text_string(&value);
    }
    bool isString() const
    {
        return valid && isString(it[depth]);
    }
    bool isBytes(const CborValue &value) const
    {
        return cbor_value_is_byte_string(&value);
    }
    bool isBytes() const
    {
        return valid && isBytes(it[depth]);
    }
    bool isInteger(const CborValue &value) const
    {
        return cbor_value_is_integer(&value);
    }
    bool isInteger() const
    {
        return valid && isInteger(it[depth]);
    }
    bool isFloat(const CborValue &value) const
    {
        return cbor_value_is_float(&value);
    }
    bool isFloat() const
    {
        return valid && isFloat(it[depth]);
    }
    bool isBoolean(const CborValue &value) const
    {
        return cbor_value_is_boolean(&value);
    }
    bool isBoolean() const
    {
        return valid && isBoolean(it[depth]);
    }
    bool isNull(const CborValue &value) const
    {
        return cbor_value_is_null(&value);
    }
    bool isNull() const
    {
        return valid && isNull(it[depth]);
    }
    template <typename T>
    bool getINT(CborValue &value, T &v)
    {
        static_assert(std::is_integral_v<T>, "Integer type required");
        if (cbor_value_at_end(&value))return false;
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
        return valid && getINT(it[depth], v);
    }
    template <typename T>
    bool getFLOAT(CborValue &value, T &v)
    {
        static_assert(std::is_floating_point_v<T>,
                      "Floating-point type required");
        if (cbor_value_at_end(&value))return false;
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
        return valid && getFLOAT(it[depth], v);
    }
    bool getBOOL(CborValue &value, bool &v)
    {
        if (cbor_value_at_end(&value))return false;
        if (!cbor_value_is_boolean(&value))
            return false;
        if (cbor_value_get_boolean(&value, &v) != CborNoError)
            return false;
        cbor_value_advance_fixed(&value);
        return true;
    }
    bool getBOOL(bool &v)
    {
        return valid && getBOOL(it[depth], v);
    }
    bool getSTRING(CborValue &value, etl::string_view &out)
    {
        if (cbor_value_at_end(&value))return false;
        if (!cbor_value_is_text_string(&value))
            return false;
        const char *data;
        size_t size;
        if (cbor_value_get_text_string_chunk(&value, &data, &size) != CborNoError)
            return false;
        out = etl::string_view(data, size);
        cbor_value_advance(&value);
        return true;
    }
    bool getSTRING(etl::string_view &out)
    {
        return valid && get(it[depth], out);
    }
    bool getBYTES(CborValue &value, etl::span<const uint8_t> &out)
    {
        if (cbor_value_at_end(&value))return false;
        if (!cbor_value_is_byte_string(&value))
            return false;
        const uint8_t *data;
        size_t size;
        if (cbor_value_get_byte_string_chunk(&value, &data, &size) != CborNoError)
            return false;
        out = etl::span<const uint8_t>(data, size);
        cbor_value_advance(&value);
        return true;
    }
    bool getBYTES(etl::span<const uint8_t> &out)
    {
        return valid && get(it[depth], out);
    }
    bool getNull(CborValue &value)
    {
        if (cbor_value_at_end(&value))return false;
        if (!cbor_value_is_null(&value))
            return false;
        cbor_value_advance_fixed(&value);
        return true;
    }
    bool getNull()
    {
        return valid && getNull(it[depth]);
    }
    template <typename T>
    bool get(CborValue &value, T &v)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            return getBOOL(value, v);
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return getINT(value, v);
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            return getFLOAT(value, v);
        }
        else if constexpr (std::is_same_v<T, etl::string_view>)
        {
            return getSTRING(value, v);
        }
        else if constexpr (std::is_same_v<T, etl::span<const uint8_t>>)
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
        return valid && get(it[depth], v);
    }
    CborType getType(CborValue &value)
    {
        switch (cbor_value_get_type(&value))
        {
        case CborIntegerType:
            return CborType::Integer;
        case CborByteStringType:
            return CborType::ByteString;
        case CborTextStringType:
            return CborType::TextString;
        case CborArrayType:
            return CborType::Array;
        case CborMapType:
            return CborType::Map;
        case CborTagType:
            return CborType::Tag;
        case CborSimpleType:
            return CborType::Simple;
        case CborBooleanType:
            return CborType::Boolean;
        case CborNullType:
            return CborType::Null;
        case CborUndefinedType:
            return CborType::Undefined;
        case CborHalfFloatType:
            return CborType::HalfFloat;
        case CborFloatType:
            return CborType::Float;
        case CborDoubleType:
            return CborType::Double;
        case CborInvalidType:
            return CborType::Invalid;
        }

        return CborType::Invalid;
    }
    CborType getType() { return getType(it[depth]); }
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
    bool enterContainer(CborType type)
    {
        if (!valid || depth >= MAX_NESTING)
            return false;

        CborValue &parent = it[depth];
        size_t len;

        if (type == CborType::Array)
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
        return enterContainer(CborType::Array);
    }
    bool enterMap()
    {
        return enterContainer(CborType::Map);
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
               container_type[depth - 1] == CborType::Array &&
               leaveContainer();
    }
    bool leaveMap()
    {
        return depth > 0 &&
               container_type[depth - 1] == CborType::Map &&
               leaveContainer();
    }
    bool containerLength(size_t &length) const
    {
        if (!depth)
            return false;

        length = container_length[depth - 1];
        return true;
    }
    bool find(uint8_t key, CborValue &it)
    {
    }
    bool find(const char *str, CborValue &result) const
    {
        if (!valid || depth == 0 ||
            container_type[depth - 1] != CborType::Map)
            return false;

        if (cbor_value_map_find_value(&it[depth], str, &result) != CborNoError)
            return false;

        return cbor_value_is_valid(&result);
    }
};

struct CborEncode
{
    CborParser encoder;
    CborValue it[MAX_NESTING + 1];
}