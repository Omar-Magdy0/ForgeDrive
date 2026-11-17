#pragma once
#include "etl/tuple.h"
#include "PmsmControlTypes.h"
#include "Rpc.h"


#define PMSM_MODEL_FIELDS(X) \
    X(0, R),                 \
    X(1, Ld),                \
    X(2, Lq),                \
    X(3, psi),               \
    X(4, J),                 \
    X(5, B),                 \
    X(6, pole_pairs)

#define SET_SETPOINT_ARGS(X) \
    X(0, setpoint, float),   \
    X(1, mode, MechMode)
#define SET_SETPOINT_RET(X) \
    X(0, error, PMSM_ERR)

#define SET_MODEL_ARGS(X) \
    X(0, model, PmsmModel)
#define SET_MODEL_RET(X) \
    X(0, error, PMSM_ERR)

#define GET_MODEL_ARGS(X)
#define GET_MODEL_RET(X) \
    X(0, model, PmsmModel)

TUPLE_OF(PmsmModel, PMSM_MODEL_FIELDS)

RPC_SCHEMA(setSetpoint, 0, SET_SETPOINT_ARGS, SET_SETPOINT_RET)
RPC_SCHEMA(setModel,    1, SET_MODEL_ARGS,    SET_MODEL_RET)
RPC_SCHEMA(getModel,    2, GET_MODEL_ARGS,    GET_MODEL_RET)

namespace ForgeDriveMC{

class RpcCall final : public Rpc::Call<RpcCall>
{
public:
    RpcCall(void *err_ctx, void (*err_cb)(void *, const void *))
        : Call<RpcCall>(err_ctx, err_cb)
    {}

    bool callImpl(uint16_t id)
    {
        switch (id)
        {
            RPC_CALL_METHOD(setSetpoint)
            RPC_CALL_METHOD(setModel)
            RPC_CALL_METHOD(getModel)

        default:
            return false;
        }
    }
};


};