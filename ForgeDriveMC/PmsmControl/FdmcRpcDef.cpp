#include "RpcDef.h"
#include "Sys.h"
using namespace Fdmc;
#define RPC_DEF(m, op)                                     \
  Rpc::DispatchSts Fdmc::m(Rpc::DispatchStack stack) {     \
    using Arg = MethodTraits<Fdmc::Id::Id_##m>::Arg;       \
    using Ret = MethodTraits<Fdmc::Id::Id_##m>::Ret;       \
    Arg& arg  = *reinterpret_cast<Arg*>(stack.arg.data()); \
    Ret& ret  = *reinterpret_cast<Ret*>(stack.ret.data()); \
    op                                                     \
  }
RPC_DEF(SetSetpoint, ret.err = Sys::pmsm1.SetSetpoint(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetSetpoint, ret = Sys::pmsm1.GetSetpoint(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetModel, ret.err = Sys::pmsm1.SetModel(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetModel, ret = Sys::pmsm1.GetModel(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetControlMode, ret.err = Sys::pmsm1.SetControlMode(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetControlMode, ret = Sys::pmsm1.GetControlMode(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetConfigPwm, ret.err = Sys::pmsm1.SetConfigPwm(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetConfigPwm, ret = Sys::pmsm1.GetConfigPwm(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetConfigAnalog, ret.err = Sys::pmsm1.SetConfigAnalog(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetConfigAnalog, ret = Sys::pmsm1.GetConfigAnalog(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetPid, ret.err = Sys::pmsm1.SetPid(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetPid, ret = Sys::pmsm1.GetPid(arg.n); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetConfigScomm, ret.err = Sys::pmsm1.SetConfigScomm(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetConfigScomm, ret = Sys::pmsm1.GetConfigScomm(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(ClearFault, ret.err = Sys::pmsm1.ClearFault(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetFault, ret.fault = Sys::pmsm1.GetFault(); return Rpc::DispatchSts::DispatchSts_Ok;)
#undef RPC_DEF