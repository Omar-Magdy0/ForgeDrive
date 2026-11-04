#pragma once
#include <stdint.h>
#include "PmsmControlCore.h"
#include "TCbor.hpp"
#include "sys.h"

namespace ForgeDriveMC_RPC
{

    enum class RPCStatus : uint8_t
    {
        OK,
        INVALID_ARGUMENTS,
        METHOD_NOT_FOUND,
        EXECUTION_ERROR,
        BUSY
    };

    enum class RPC_ID : uint16_t
    {
        SET_SETPOINT = 0,
    };

    bool RPC_ERROR(TCborParse &parse, TCborEncode &encode, RPCStatus sts)
    {
        encode.put(UINT16_MAX);
        encode.put(sts);
        return false;
    }

    bool RPC_SET_SETPOINT(TCborParse &parse, TCborEncode &encode, RPCStatus &sts)
    {
        float sp;
        MechMode mc;
        PMSM_ERR err;
        if (!(parse.get(sp) && parse.get(mc)))
            return RPC_ERROR(parse, encode, RPCStatus::INVALID_ARGUMENTS);
        err = Sys::pmsm1.setSetpoint(sp, mc);
        encode.put(RPC_ID::SET_SETPOINT);
        encode.put(err);
        return true;
    }

    bool RPC_DISPATCH(TCborParse &parse, TCborEncode &encode)
    {
        RPCStatus sts = RPCStatus::OK;
        RPC_ID id;
        bool suc = true;
        if (!parse.get(id))
            return RPC_ERROR(parse, encode, RPCStatus::METHOD_NOT_FOUND);
        if (!parse.enterArray())
            return RPC_ERROR(parse, encode, RPCStatus::INVALID_ARGUMENTS);
        switch (id)
        {
        case RPC_ID::SET_SETPOINT:
            suc = RPC_SET_SETPOINT(parse, encode, sts);
            break;
        default:
            return RPC_ERROR(parse, encode, RPCStatus::METHOD_NOT_FOUND);
        }
        if (!parse.leaveArray())
            return RPC_ERROR(parse, encode, RPCStatus::INVALID_ARGUMENTS);
        return suc;
    }

};