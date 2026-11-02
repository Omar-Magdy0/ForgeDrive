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

/*
class DAQSessionAPP : public daq::Session
{
protected:
    abf::Stream &abf;

public:
    DAQSessionAPP(abf::Stream &abf_stream,
                  const char *Meta,
                  etl::span<uint8_t> idv_buf) : daq::Session(Meta, idv_buf), abf(abf_stream)
    {
    }
    uint8_t send() override
    {
        uint8_t abf_header[abf::HEADER_SIZE];
        abf.encode(abf_header, writer.data(), 16, writer.size());
        fd_usbxch_write(USB1, abf_header, abf::HEADER_SIZE);
        fd_usbxch_write(USB1, writer.data(), writer.size());
        fd_usbxch_flush(USB1);
        return 0;
    }
    uint8_t on_mark(daq::MARKER mark, bool entry) override
    {
        switch (mark)
        {
        case daq::MARKER::AUTO_DISCOVER_REQ:
            if (!entry)
                discovery_respond();
            break;

        default:
            break;
        }
        return 0;
    }
};
*/
class Sys
{
    static constexpr unsigned int ABFRX_BUF_SIZE = 255;
    static constexpr size_t LCOM_STACK_SIZE = 256; // lcom denotes, low priority communication (usb telemetry/commanding)
    static constexpr unsigned int LCOM_PRIO = 3;

    static inline void onFrame(void *ctx, uint8_t id, uint8_t *payload, uint8_t payload_len)
    {
    }
    static inline void onError(void *ctx, uint8_t id)
    {
    }
    static inline void onPmsmFault(void *ctx, PMSM_FAULT f)
    {
    }
    static ConfigGen configGen;
    inline static uint8_t abfrx_buf[ABFRX_BUF_SIZE];
    inline static StaticTask_t lcom_buf;
    inline static StackType_t lcom_stack[LCOM_STACK_SIZE];
    inline static abf::Stream abfStream = abf::Stream(abfrx_buf, ABFRX_BUF_SIZE, NULL, onFrame, onError);

public:
    inline static PmsmControlCore pmsm1 = PmsmControlCore(onPmsmFault, configGen);
    static void init(void);
    inline static TaskHandle_t lcom_handle = nullptr;
};
