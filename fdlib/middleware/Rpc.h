#pragma once
#include "preprocessor.h"
#include "etl/tuple.h"
#include "etl/utility.h"
#include <stdint.h>
#include "TCbor.hpp"

#ifndef SCHEMA_DESCRIPTORS
#define SCHEMA_DESCRIPTORS 1
#endif

#if SCHEMA_DESCRIPTORS
#define FIELD_DESCRIPTOR(key, name) \
    FieldDescriptor { key, #name }
#else
#define FIELD_DESCRIPTOR(key, name) \
    FieldDescriptor { key }
#endif

struct FieldDescriptor
{
    uint8_t key;
#if SCHEMA_DESCRIPTORS
    const char *name;
#endif
};


template <typename T>
struct TupleOf
{
    static inline constexpr bool value = false;
};

#define FIELD_TIE(key, name) v.name
#define TUPLE_OF(type, fields_list)             \
    template <>                                 \
    struct TupleOf<type>                        \
    {                                           \
        static constexpr bool value = true;     \
        static constexpr const char *name()     \
        {                                       \
            return #type;                       \
        }                                       \
                                                \
        static constexpr auto fields()          \
        {                                       \
            return etl::make_tuple(             \
                fields_list(FIELD_DESCRIPTOR)); \
        }                                       \
                                                \
        template <typename T>                   \
        static auto tie(T &&v)                  \
        {                                       \
            return etl::tie(                    \
                fields_list(FIELD_TIE));        \
        }                                       \
    };

template <uint16_t ID, typename ArgsT, typename ReturnT>
struct RPCSchema
{
    using Args   = ArgsT;
    using Return = ReturnT;
    static constexpr uint16_t id = ID;
};

#define RPC_TYPE(key, name, type) type
#define RPC_DESCRIPTOR(key, name, type) FIELD_DESCRIPTOR(key, name)

#define RPC_SCHEMA(rpc_name, ID, args_list, ret_list)       \
struct rpc_name : RPCSchema<                                \
    ID,                                                     \
    etl::tuple<args_list(RPC_TYPE)>,                        \
    etl::tuple<ret_list(RPC_TYPE)>                           \
>                                                          \
{                                                          \
    static constexpr const char* name()                    \
    {                                                      \
        return #rpc_name;                                  \
    }                                                      \
                                                           \
    static constexpr auto argFields()                      \
    {                                                      \
        return etl::make_tuple(args_list(RPC_DESCRIPTOR)); \
    }                                                      \
                                                           \
    static constexpr auto retFields()                      \
    {                                                      \
        return etl::make_tuple(ret_list(RPC_DESCRIPTOR));  \
    }                                                      \
};

#define RPC_DISPATCH_METHOD(method) \
    case method::id:                \
        method();                   \
        return true;

#define RPC_IMPL(method, code)                          \
    bool method()                                       \
    {                                                   \
        method::Args args;                              \
        method::Return result;                          \
        if (!serdes.deserialize(args))                  \
            return ERROR(Rpc::DispatchStatus::INVALID_ARGUMENTS); \
        code;                                           \
        if (!serdes.serialize(method::id))              \
            return ERROR(Rpc::DispatchStatus::INTERNAL_ERROR);    \
        return serdes.serialize(result);                \
    }

#define RPC_CALL_METHOD(method)                                                            \
    case method::id:                                                                       \
    {                                                                                      \
        method::Return result;                                                             \
        ccall.cb(ccall.ctx, serdes.deserialize(result) ? static_cast<void *>(&result) : nullptr); \
        return true;                                                                             \
    }

namespace Rpc
{

#define RPC_CALL_STATUS_LIST(X) \
    X(IDLE, 0)                  \
    X(PENDING, 1)

#define RPC_DISPATCH_STATUS_LIST(X) \
    X(OK, 0)                        \
    X(INVALID_ARGUMENTS, 1)         \
    X(METHOD_NOT_FOUND, 2)          \
    X(EXECUTION_ERROR, 3)           \
    X(INTERNAL_ERROR, 4)            \
    X(BUSY, 5)

    FD_ENUM_STRUCT(CallStatus, uint8_t, RPC_CALL_STATUS_LIST)
    FD_ENUM_STRUCT(DispatchStatus, uint8_t, RPC_DISPATCH_STATUS_LIST)


    struct callable
    {
        void *ctx;
        void (*cb)(void *, const void *);
    };

    struct Serializer
    {
        TCborParse parse{etl::span<const uint8_t>{}};
        TCborEncode encode{etl::span<uint8_t>{}};

        void SetInput(etl::span<const uint8_t> rx)
        {
            parse = TCborParse(rx);
        }
        void SetOutput(etl::span<uint8_t> tx)
        {
            encode = TCborEncode(tx);
        }

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
        size_t size() const
        {
            return encode.size();
        }
    };

    template <typename Derived>
    class Dispatch
    {
    protected:
        uint16_t id;
        Serializer serdes;
        bool error = false;
        bool ERROR(DispatchStatus err)
        {
            error = true;
            serdes.serialize(UINT16_MAX);
            auto ret = etl::tuple{id, err};
            serdes.serialize(ret);
            return false;
        }

    public:
        Dispatch() = default;
        bool dispatch(etl::span<const uint8_t> rx, etl::span<uint8_t> tx)
        {
            serdes.SetInput(rx);
            serdes.SetOutput(tx);
            error = false;
            uint16_t id_temp = UINT16_MAX;
            id = UINT16_MAX;
            if (!serdes.deserialize(id_temp))
                return ERROR(DispatchStatus::METHOD_NOT_FOUND);
            id = id_temp;
            bool found = static_cast<Derived *>(this)->dispatchImpl(id);
            if (!found)
                ERROR(DispatchStatus::METHOD_NOT_FOUND);
            return !error;
        }
        size_t size() { return serdes.size(); }
    };

    template <typename Derived>
    class Call
    {
    protected:
        callable err;
        callable ccall;
        Serializer serdes;

    public:
        Call(void *err_ctx, void (*err_cb)(void *, const void *))
        {
            err.cb = err_cb;
            err.ctx = err_ctx;
        };
        template <typename Method>
        bool call(const typename Method::Args &args, etl::span<uint8_t> tx, void *_ctx, void (*_cb)(void *, const void *))
        {
            serdes.SetOutput(tx);
            if (!serdes.serialize(Method::id) || !serdes.serialize(args))
                return false;
            ccall.cb = _cb;
            ccall.ctx = _ctx;
            return true;
        }
        bool process(etl::span<const uint8_t> rx)
        {
            serdes.SetInput(rx);
            uint16_t id;
            bool found = false;
            if (!serdes.deserialize(id))
                return found;
            if (id == UINT16_MAX)
            {
                found = true;
                etl::tuple<uint16_t, DispatchStatus> result;
                err.cb(err.ctx, serdes.deserialize(result) ? static_cast<void *>(&result) : nullptr);
                return found;
            }
            found = static_cast<Derived *>(this)->callImpl(id);
            return found;
        }
    };

};
