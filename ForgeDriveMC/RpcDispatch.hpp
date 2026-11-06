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

#define RPC_METHOD(method)           \
    case RpcSchema::RPC_ID::method: \
        suc = method();              \
        break;

    struct RpcDispatch
    {
    private:
        TCborParse parse;
        TCborEncode encode;
        bool ERROR(RPCStatus err)
        {
            encode.put(UINT16_MAX);
            encode.enterArray(1);
            encode.put(err);
            encode.leaveArray();
            return false;
        }
        template <typename T>
        inline bool serialize(const T &val)
        {
            if constexpr (etl::is_tuple<T>::value)
                return serializeTuple(val);
            else
                return encode.put(val);
        }
        template <typename T>
        inline bool deserialize(T &val)
        {
            if constexpr (etl::is_tuple<T>::value)
                return deserializeTuple(val);
            else
                return parse.get(val);
        }
        template <typename Tuple>
        bool serializeTuple(const Tuple &args)
        {
            if (!encode.enterArray(etl::tuple_size<Tuple>::value))
                return ERROR(RPCStatus::INTERNAL_ERROR);
            if (!etl::apply([&](const auto &...values)
                            { return (serialize(values) && ...); }, args))
                return ERROR(RPCStatus::INTERNAL_ERROR);
            if (!encode.leaveArray())
                return ERROR(RPCStatus::INTERNAL_ERROR);
            return true;
        }
        template <typename Tuple>
        bool deserializeTuple(Tuple &args)
        {
            if (!parse.enterArray())
                return ERROR(RPCStatus::INVALID_ARGUMENTS);
            if (!etl::apply([&](auto &...values)
                            { return (deserialize(values) && ...); }, args))
                return ERROR(RPCStatus::INVALID_ARGUMENTS);
            if (!parse.leaveArray())
                return ERROR(RPCStatus::INVALID_ARGUMENTS);
            return true;
        }
        bool setSetpoint()
        {
            setSetpoint::Args args;
            setSetpoint::Return result;
            if (!deserialize(args))
                return false;
            etl::get<0>(result) = Sys::pmsm1.setSetpoint(etl::get<0>(args), etl::get<1>(args));
            if (!serialize(setSetpoint::id))
                return ERROR(RPCStatus::INTERNAL_ERROR);
            return serialize(result);
        }
        bool setModel()
        {
            return true;
        }
        bool setControlMode()
        {
            return true;
        }

    public:
        RpcDispatch(etl::span<const uint8_t> rx, etl::span<uint8_t> tx) : parse(rx), encode(tx) {}
        size_t dispatch()
        {
            RPC_ID id;
            bool suc = true;
            if (!deserialize(id))
                return ERROR(RPCStatus::METHOD_NOT_FOUND);

            switch (id)
            {
                RPC_METHOD(setSetpoint);
                RPC_METHOD(setControlMode);
                RPC_METHOD(setModel);
            default:
                return ERROR(RPCStatus::METHOD_NOT_FOUND);
            }
            return suc;
        }
        size_t size() { return encode.size(); }
    };

};