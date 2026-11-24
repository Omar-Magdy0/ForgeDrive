#pragma once

namespace Rpc
{
    enum RpcType
    {
        RpcType_NONE = 0,
        RpcType_ERROR = 1,
        RpcType_RESPONSE = 2,
        RpcType_NOTIFY = 3,
    };

    enum DispatchStatus : uint8_t
    {
        DispatchStatus_OK = 0,
        DispatchStatus_INVALID_ARGUMENTS = 1,
        DispatchStatus_NOT_FOUND = 2,
        DispatchStatus_EXECUTION_ERROR = 3,
        DispatchStatus_INTERNAL_ERROR = 4,
        DispatchStatus_REGISTRY_INVALID = 5,
        DispatchStatus_TIMEOUT = 6,
    };

    inline const char *dispatchStatusToString(DispatchStatus status)
    {
        switch (status)
        {
        case DispatchStatus_OK:
            return "OK";
        case DispatchStatus_INVALID_ARGUMENTS:
            return "Invalid arguments";
        case DispatchStatus_NOT_FOUND:
            return "Method not found";
        case DispatchStatus_EXECUTION_ERROR:
            return "Execution error";
        case DispatchStatus_INTERNAL_ERROR:
            return "Internal error";
        case DispatchStatus_REGISTRY_INVALID:
            return "Registry invalid";
        case DispatchStatus_TIMEOUT:
            return "Timeout";
        default:
            return "Unknown error";
        }
    }
}