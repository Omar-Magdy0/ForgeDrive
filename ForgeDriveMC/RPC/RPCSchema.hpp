#pragma once
#include "etl/tuple.h"
#include "PmsmControlTypes.h"


namespace RpcSchema{

template <typename T>
struct TupleOf
{
    static inline constexpr bool value = false;
};

template <>
struct TupleOf<PmsmModel>
{
    static inline constexpr bool value = true;
    template <typename T>
    static auto tie(T&& v)
    {
        return etl::tie(v.R, v.Ld, v.Lq, v.psi, v.J, v.B, v.pole_pairs);
    }
};


template <uint16_t ID, typename ArgsT, typename ReturnT>
struct RPCSchema
{
    using Args   = ArgsT;
    using Return = ReturnT;
    static constexpr uint16_t id = ID;
};



using setSetpoint = RPCSchema<
    0,
    etl::tuple<float, MechMode>,
    etl::tuple<PMSM_ERR>
>;

using setModel = RPCSchema<
    1,
    etl::tuple<PmsmModel>,
    etl::tuple<PMSM_ERR>
>;

using getModel = RPCSchema<
    2,
    etl::tuple<>,
    etl::tuple<PmsmModel>
>;


};