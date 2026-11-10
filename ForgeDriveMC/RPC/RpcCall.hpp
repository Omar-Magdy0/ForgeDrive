#pragma once
#include <stdint.h>
#include "etl/tuple.h"
#include "TCbor.hpp"
#include "sys.h"
#include "RPCSchema.hpp"
#include "etl/utility.h"

using namespace RpcSchema;
struct RpcCall
{
    enum Status : uint8_t
    {
        IDLE,
        PENDING,
    };

    struct callable
    {
        void *ctx;
        void (*cb)(void *, const void *);
    };

private:
    TCborParse parse{etl::span<const uint8_t>{}};
    TCborEncode encode{etl::span<uint8_t>{}};
    callable err;
    callable ccall;
    template <typename T>
    inline bool serialize(const T &val)
    {
        using U = std::remove_cv_t<T>;
        if constexpr (etl::is_tuple<U>::value)
            return serializeTuple(val);
        else if constexpr (TupleOf<T>::value)
            return serializeTuple(TupleOf<T>::tie(val));
        else
            return encode.put(val);
    }
    template <typename Tuple>
    inline bool serializeTuple(const Tuple &val)
    {
        if (!encode.enterArray(etl::tuple_size<Tuple>::value))
            return false;
        if (!etl::apply([this](const auto &...values)
                        { return (serialize(values) && ...); }, val))
            return false;
        if (!encode.leaveArray())
            return false;
        return true;
    }
    template <typename T>
    inline bool deserialize(T &val)
    {
        using U = std::remove_cv_t<T>;
        if constexpr (etl::is_tuple<U>::value)
            return deserializeTuple(val);
        else if constexpr (TupleOf<T>::value)
            return deserializeTuple(TupleOf<T>::tie(val));
        else
            return parse.get(val);
    }
    template <typename T>
    inline bool deserializeTuple(T &&val)
    {
        if (!parse.enterArray())
            return false;
        if (!etl::apply([this](auto &...values)
                        { return (deserialize(values) && ...); }, val))
            return false;
        if (!parse.leaveArray())
            return false;
        return true;
    }

#define RPC_CALL_METHOD(method)                                                            \
    case method::id:                                                                       \
    {                                                                                      \
        method::Return result;                                                             \
        ccall.cb(ccall.ctx, deserialize(result) ? static_cast<void *>(&result) : nullptr); \
        break;                                                                             \
    }

public:
    RpcCall(void *err_ctx, void (*err_cb)(void *, const void *))
    {
        err.cb = err_cb;
        err.ctx = err_ctx;
    };
    template <typename Method>
    bool call(const typename Method::Args &args, etl::span<uint8_t> tx, void *_ctx, void (*_cb)(void *, const void *))
    {
        encode = TCborEncode(tx);
        if (!serialize(Method::id) || !serialize(args))
            return false;
        ccall.cb = _cb;
        ccall.ctx = _ctx;
        return true;
    }
    bool RpcProcess(etl::span<const uint8_t> rx)
    {
        parse = TCborParse(rx);
        uint16_t id;
        if (!deserialize(id))
            return false;
        switch (id)
        {
            RPC_CALL_METHOD(setSetpoint);
            RPC_CALL_METHOD(setModel);
            RPC_CALL_METHOD(getModel);

        case UINT16_MAX: // ERRORS_HANDLING
        {
            etl::tuple<uint16_t, uint8_t> result;
            err.cb(err.ctx, deserialize(result) ? static_cast<void *>(&result) : nullptr);
            break;
        }
        default:
            break;
        }
        return true;
    }
};
