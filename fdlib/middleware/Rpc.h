#pragma once
#include "preprocessor.h"
#include "traits.h"
#include "etl/tuple.h"
#include "etl/utility.h"
#include <stdint.h>



namespace Rpc
{
using idType = uint16_t;

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
        void (*cb)(void *ctx, void *arg, void *ret);
    };


    struct RPCMessage {
        idType id;
        uint8_t type;
        uint8_t *payload;
        size_t size;
    };


    template<typename Derived>
    class Rpc 
    {
        idType id;
        public:
        bool Error(DispatchStatus err)
        {

            return false;
        };

        bool dispatch(etl::span<const uint8_t> rx, etl::span<uint8_t> tx)
        {

            return true;
        }
    };

};
