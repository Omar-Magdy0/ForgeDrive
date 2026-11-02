#pragma once
#include <stdint.h>
#include "PmsmControlCore.h"

namespace ForgeDriveMC_RPC
{

    enum class RPCMessageType : uint8_t
    {
        Request = 0,
        Response,
        Notification
    };

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


    bool RPC_DISPATCH(RPC_CBOR &cbor)
    {
        RPCStatus sts = RPCStatus::OK;
        cbor.reset();
        uint8_t id;
        if(!cbor.get(id))sts = RPCStatus::METHOD_NOT_FOUND; 
        if (sts == RPCStatus::OK)
        {
            switch ((RPC_ID)id)
            {
            case SET_SETPOINT:
            {
                float sp;
                uint8_t mc;
                PMSM_ERR err;
                bool arg_good = true;
                arg_good &= cbor.get(sp);
                arg_good &= cbor.get(mc);
                arg_good &= cbor.atEnd();
                if(!arg_good){sts = RPCStatus::INVALID_ARGUMENTS;}
                if(arg_good)err = setSetpoint(sp, static_cast<MCMode>mc);

                break;
            }
            default:
                sts = RPCStatus::METHOD_NOT_FOUND;
                break;
            }
        }
        if (sts != RPCStatus::OK)
        {
            switch (sts)
            {
            case RPCStatus::INVALID_ARGUMENTS:
            {
                break;
            }
            case RPCStatus::METHOD_NOT_FOUND:
            {
                break;
            }
            default:
                break;
            }
        }
        return true;
    }

};