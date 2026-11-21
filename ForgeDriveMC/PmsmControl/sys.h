#pragma once

#include "PmsmControl/PmsmControlCore.h"
#include "platform.h"
#include <cstdint>
#include <array>
#include <string.h>
#include <stdio.h>
//=======================================================
#include "ABFStream.h"
#include "fd_usbxch.h"
#include <etl/span.h>
#include "fd_drv.h"
#include "Rpc.h"
#include "fdmc.rpc.h"
#ifdef __cplusplus
extern "C" {
#endif 
#include "FreeRTOS.h"
#include "task.h"
#ifdef __cplusplus
}
#endif 

void onFrame(void *ctx, uint8_t id, uint8_t *payload, uint8_t payload_len);
void onError(void *ctx, uint8_t id);
void onPmsmFault(void *, PmsmFault err);

class Sys
{
    static ConfigGen configGen;
    //lcom_begin
    inline static StaticTask_t lcom_buf;
    static constexpr unsigned int LCOM_PRIO = 3;
    static constexpr size_t LCOM_STACK_SIZE = 256; // lcom denotes, low priority communication (usb telemetry/commanding)
    inline static StackType_t lcom_stack[LCOM_STACK_SIZE];
    //lcom_end
public:
    inline static PmsmControlCore pmsm1 = PmsmControlCore(onPmsmFault, configGen);
    static void init(void);
    inline static TaskHandle_t lcom_handle = nullptr;
};
