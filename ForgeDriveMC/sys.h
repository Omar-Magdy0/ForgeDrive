#pragma once

#include "PmsmControl/PmsmControlCore.h"
#include "platform.h"
#include "fd_usbxch.h"
#include <cstdint>
#include <array>
#include <string.h>
#include <stdio.h>
//=======================================================
#include "ABFStream.h"
#include "fd_usbxch.h"
#include <etl/span.h>
#include "fd_drv.h"

#ifdef __cplusplus
extern "C" {
#endif 
#include "FreeRTOS.h"
#include "task.h"
#ifdef __cplusplus
}
#endif 


class Sys
{
    static constexpr size_t LCOM_STACK_SIZE = 256; // lcom denotes, low priority communication (usb telemetry/commanding)
    static constexpr unsigned int LCOM_PRIO = 3;

    static void onFrame(void *ctx, uint8_t id, uint8_t *payload, uint8_t payload_len);
    static void onError(void *ctx, uint8_t id);
    static void onPmsmFault(void *ctx, PMSM_FAULT f);
    static ConfigGen configGen;
    inline static StaticTask_t lcom_buf;
    inline static StackType_t lcom_stack[LCOM_STACK_SIZE];
    static constexpr unsigned int ABFRX_BUF_SIZE = 255;
    static constexpr unsigned int TX_BUF_SIZE = 255;
    inline static uint8_t abfrx_buf[ABFRX_BUF_SIZE];
public:
    inline static PmsmControlCore pmsm1 = PmsmControlCore(onPmsmFault, configGen);
    static void init(void);
    inline static TaskHandle_t lcom_handle = nullptr;
    inline static abf::Stream abfStream = abf::Stream(abfrx_buf, ABFRX_BUF_SIZE, nullptr, onFrame, onError);
    inline static uint8_t tx_buf[TX_BUF_SIZE];
    inline static uint8_t abf_header_buf[abf::HEADER_SIZE];
};
