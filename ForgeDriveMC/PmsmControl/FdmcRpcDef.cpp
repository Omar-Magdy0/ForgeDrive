#include "sys.h"
using namespace Fdmc;
#define RPC_DEF(m, op)                                     \
  Rpc::DispatchStatus Fdmc::m(Rpc::DispatchStack stack) {  \
    using Arg = MethodTraits<Fdmc::Id::Id_##m>::Arg;       \
    using Ret = MethodTraits<Fdmc::Id::Id_##m>::Ret;       \
    Arg& arg  = *reinterpret_cast<Arg*>(stack.arg.data()); \
    Ret& ret  = *reinterpret_cast<Ret*>(stack.ret.data()); \
    op                                                     \
  }
RPC_DEF(SetSetpoint, {
  ret.err = Sys::pmsm1.setSetpoint(arg.sp, arg.mech_mode);
  return Rpc::DispatchStatus::DispatchStatus_OK;
})
RPC_DEF(SetModel, {
  ret.err = Sys::pmsm1.setModel(arg);
  return Rpc::DispatchStatus::DispatchStatus_OK;
})
Rpc::DispatchStatus Fdmc::SetControlMode(Rpc::DispatchStack stack) { return {}; }
Rpc::DispatchStatus Fdmc::SetConfigPwm(Rpc::DispatchStack stack) { return {}; }
Rpc::DispatchStatus Fdmc::SetConfigAnalog(Rpc::DispatchStack stack) { return {}; }
Rpc::DispatchStatus Fdmc::SetPid(Rpc::DispatchStack stack) { return {}; }
Rpc::DispatchStatus Fdmc::SetConfigSComm(Rpc::DispatchStack stack) { return {}; }
Rpc::DispatchStatus Fdmc::ClearFault(Rpc::DispatchStack stack) { return {}; }
Rpc::DispatchStatus Fdmc::GetControlMode(Rpc::DispatchStack stack) { return {}; }
RPC_DEF(GetModel, ret = Sys::pmsm1.getModel(); return Rpc::DispatchStatus::DispatchStatus_OK;)
Rpc::DispatchStatus Fdmc::GetPid(Rpc::DispatchStack stack) { return {}; }
Rpc::DispatchStatus Fdmc::GetFault(Rpc::DispatchStack stack) { return {}; }
#undef RPC_DEF