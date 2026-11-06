#pragma once
#include "etl/tuple.h"
#include "PmsmControlCore.h"

namespace RpcSchema{



enum class RPC_ID : uint16_t
{
    setSetpoint = 0,
    setModel = 1,
    setControlMode = 2,
};

template <RPC_ID ID, typename ArgsT, typename ReturnT>
struct RPCSchema
{
    using Args   = ArgsT;
    using Return = ReturnT;
    static constexpr RPC_ID id = ID;
};



using setSetpoint = RPCSchema<
    RPC_ID::setSetpoint,
    etl::tuple<float, MechMode>,
    etl::tuple<PMSM_ERR>
>;



};