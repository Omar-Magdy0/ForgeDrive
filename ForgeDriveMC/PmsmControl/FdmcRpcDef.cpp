#include "sys.h"

using namespace Fdmc;

#define RPC_ENTRY(m) \
    using Arg = MethodTraits<Fdmc::Id::Id_##m>::Arg; \
    using Ret = MethodTraits<Fdmc::Id::Id_##m>::Ret; \
    Arg& arg = *reinterpret_cast<Arg*>(stack.arg.data()); \
    Ret& ret = *reinterpret_cast<Ret*>(stack.ret.data());

Rpc::DispatchStatus Fdmc::SetSetpoint(Rpc::DispatchStack stack) 
{
    RPC_ENTRY(SetSetpoint)
    Sys::pmsm1.setSetpoint(arg.sp, arg.mech_mode);
    return Rpc::DispatchStatus::DispatchStatus_OK;
}
Rpc::DispatchStatus Fdmc::SetModel(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::SetControlMode(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::SetConfigPwm(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::SetConfigAnalog(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::SetPid(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::SetConfigSComm(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::ClearFault(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::GetControlMode(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::GetModel(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::GetPid(Rpc::DispatchStack) { return {}; }
Rpc::DispatchStatus Fdmc::GetFault(Rpc::DispatchStack) { return {}; }