#pragma once
#include "PmsmControl/PmsmControlCore.h"
#include "platform.h"
#include <array>
#include <cstdint>
#include <stdio.h>
#include <string.h>
//=======================================================
#include "Abf.h"
#include "Drv.h"
#include "Rpc.h"
#include "Usbxch.h"
#include "fdmc.rpc.h"
#include <etl/span.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "FreeRTOS.h"
#include "task.h"
#ifdef __cplusplus
}
#endif
void onPmsmFault(void*, PmsmFault err);
class Sys {
  static ConfigGen configGen;
  // lcom_begin
  inline static StaticTask_t    lcom_buf;
  static constexpr unsigned int LCOM_PRIO       = 3;
  static constexpr size_t       LCOM_STACK_SIZE = 256; // lcom denotes, low priority communication (usb telemetry/commanding)
  inline static StackType_t     lcom_stack[LCOM_STACK_SIZE];
  // lcom_end
public:
  inline static PmsmControlCore  pmsm1 = PmsmControlCore(onPmsmFault, configGen);
  static void                    init(void);
  inline static TaskHandle_t     lcom_handle = nullptr;
  inline static Rpc::RpcServer<> rpcServer   = Rpc::RpcServer(Fdmc::registry);
};
