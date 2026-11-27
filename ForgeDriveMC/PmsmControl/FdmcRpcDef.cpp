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
RPC_DEF(SetSetpoint, ret.err = Sys::pmsm1.setSetpoint(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetSetpoint, ret = Sys::pmsm1.getSetpoint(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetModel, ret.err = Sys::pmsm1.setModel(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetModel, ret = Sys::pmsm1.getModel(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetControlMode, ret.err = Sys::pmsm1.setControlMode(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetControlMode, ret = Sys::pmsm1.getControlMode(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetConfigPwm, ret.err = Sys::pmsm1.setConfigPwm(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetConfigPwm, ret = Sys::pmsm1.getConfigPwm(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetConfigAnalog, ret.err = Sys::pmsm1.setConfigAnalog(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetConfigAnalog, ret = Sys::pmsm1.getConfigAnalog(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetPid, ret.err = Sys::pmsm1.setPid(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetPid, ret = Sys::pmsm1.getPid(arg.n); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(SetConfigScomm, ret.err = Sys::pmsm1.setConfigScomm(arg); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetConfigScomm, ret = Sys::pmsm1.getConfigScomm(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(ClearFault, ret.err = Sys::pmsm1.clearFault(); return Rpc::DispatchSts::DispatchSts_Ok;)
RPC_DEF(GetFault, ret.fault = Sys::pmsm1.getFault(); return Rpc::DispatchSts::DispatchSts_Ok;)
#undef RPC_DEF