#pragma once
#include <stdint.h>
#include "PmsmControlCore.h"
#include "etl/tuple.h"
#include "TCbor.hpp"
#include "sys.h"
#include "RPCSchema.hpp"
#include "etl/utility.h"

using namespace RpcSchema;
namespace ForgeDriveMC_RPC
{

    enum class RPCStatus : uint8_t
    {
        OK,
        INVALID_ARGUMENTS,
        METHOD_NOT_FOUND,
        EXECUTION_ERROR,
        INTERNAL_ERROR,
        BUSY
    };

#define RPC_DISPATCH_METHOD(method) \
    case method::id:                \
        suc = method();             \
        break;

#define RPC_IMPL(method, code)                          \
    bool method()                                       \
    {                                                   \
        method::Args args;                              \
        method::Return result;                          \
        if (!deserialize(args))                         \
            return ERROR(RPCStatus::INVALID_ARGUMENTS); \
        code;                                           \
        if (!serialize(method::id))                     \
            return ERROR(RPCStatus::INTERNAL_ERROR);    \
        return serialize(result);                       \
    }

    struct RpcDispatch
    {
    private:
        TCborParse parse{etl::span<const uint8_t>{}};
        TCborEncode encode{etl::span<uint8_t>{}};
        uint16_t id;
        bool ERROR(RPCStatus err)
        {
            encode.put(UINT16_MAX);
            encode.enterArray(1);
            encode.put(id);
            encode.put(err);
            encode.leaveArray();
            return false;
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

        RPC_IMPL(setSetpoint, etl::get<0>(result) = Sys::pmsm1.setSetpoint(etl::get<0>(args), etl::get<1>(args)))
        RPC_IMPL(setModel, etl::get<0>(result) = Sys::pmsm1.setModel(etl::get<0>(args)))
        RPC_IMPL(getModel, etl::get<0>(result) = Sys::pmsm1.getModel())

    public:
        RpcDispatch() {}
        size_t dispatch(etl::span<const uint8_t> rx, etl::span<uint8_t> tx)
        {
            encode = TCborEncode(tx);
            parse = TCborParse(rx);
            uint16_t id_temp = UINT16_MAX;
            id = id_temp;
            bool suc = true;
            if (!deserialize(id_temp))
                return ERROR(RPCStatus::METHOD_NOT_FOUND);
            id = id_temp;
            switch (id)
            {
                RPC_DISPATCH_METHOD(setSetpoint);
                RPC_DISPATCH_METHOD(setModel);
                RPC_DISPATCH_METHOD(getModel);
            default:
                return ERROR(RPCStatus::METHOD_NOT_FOUND);
            }
            return suc;
        }
        size_t size() { return encode.size(); }
    };

};