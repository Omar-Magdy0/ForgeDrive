#pragma once
#include "stdint.h"
namespace Rpc {
enum RpcType : uint8_t {
  RpcType_None    = 0,
  RpcType_Err     = 0,
  RpcType_Call    = 1,
  RpcType_Notif   = 2,
  RpcType_Async   = 3,
  RpcType_Stream  = 4,
  RpcType_Control = 5,
};
inline RpcType idRpcType(uint16_t id) {
  if (id == 0) return RpcType_Err;
  const uint8_t hbits = id >> 13;
  switch (hbits) {
  case 0: return RpcType_Call;
  case 1: return RpcType_Notif;
  case 2: return RpcType_Async;
  case 3: return RpcType_Stream;
  case 4: return RpcType_Control;
  default: return RpcType_None;
  }
}
enum DispatchSts : uint8_t {
  DispatchSts_Ok              = 0,
  DispatchSts_InvalidArg      = 1,
  DispatchSts_NotFound        = 2,
  DispatchSts_ExecErr         = 3,
  DispatchSts_InternalErr     = 4,
  DispatchSts_RegistryInvalid = 5,
  DispatchSts_Timout          = 6,
};
inline const char* DispatchStsToString(DispatchSts status) {
  switch (status) {
  case DispatchSts_Ok: return "OK";
  case DispatchSts_InvalidArg: return "Invalid arguments";
  case DispatchSts_NotFound: return "Method not found";
  case DispatchSts_ExecErr: return "Execution error";
  case DispatchSts_InternalErr: return "Internal error";
  case DispatchSts_RegistryInvalid: return "Registry invalid";
  case DispatchSts_Timout: return "Timeout";
  default: return "Unknown error";
  }
}
}; // namespace Rpc