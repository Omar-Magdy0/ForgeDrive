#pragma once
namespace Rpc {
enum RpcType : uint8_t {
  RpcType_NONE    = 0,
  RpcType_ERROR   = 0,
  RpcType_CALL    = 1,
  RpcType_NOTIFY  = 2,
  RpcType_ASYNC   = 3,
  RpcType_STREAM  = 4,
  RpcType_CONTROL = 5,
};
inline RpcType idRpcType(uint16_t id) {
  if (id == 0) return RpcType_ERROR;
  const uint8_t hbits = id >> 13;
  switch (hbits) {
  case 0: return RpcType_CALL;
  case 1: return RpcType_NOTIFY;
  case 2: return RpcType_ASYNC;
  case 3: return RpcType_STREAM;
  case 4: return RpcType_CONTROL;
  default: return RpcType_NONE;
  }
}
enum DispatchStatus : uint8_t {
  DispatchStatus_OK                = 0,
  DispatchStatus_INVALID_ARGUMENTS = 1,
  DispatchStatus_NOT_FOUND         = 2,
  DispatchStatus_EXECUTION_ERROR   = 3,
  DispatchStatus_INTERNAL_ERROR    = 4,
  DispatchStatus_REGISTRY_INVALID  = 5,
  DispatchStatus_TIMEOUT           = 6,
};
inline const char* dispatchStatusToString(DispatchStatus status) {
  switch (status) {
  case DispatchStatus_OK: return "OK";
  case DispatchStatus_INVALID_ARGUMENTS: return "Invalid arguments";
  case DispatchStatus_NOT_FOUND: return "Method not found";
  case DispatchStatus_EXECUTION_ERROR: return "Execution error";
  case DispatchStatus_INTERNAL_ERROR: return "Internal error";
  case DispatchStatus_REGISTRY_INVALID: return "Registry invalid";
  case DispatchStatus_TIMEOUT: return "Timeout";
  default: return "Unknown error";
  }
}
} // namespace Rpc