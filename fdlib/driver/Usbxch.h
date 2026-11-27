#ifndef USBXCH
#define USBXCH
#include <stdint.h>
#include <stdbool.h>
#include "CRing.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @brief USB Exchange handle state flags. */
typedef enum {
    FdUsbxchFlags_None        = 0x00,
    FdUsbxchFlags_Configured  = 0x01, /**< Bus is configured (SET_CONFIGURATION received) */
    FdUsbxchFlags_Suspended   = 0x02, /**< Bus is suspended */
    FdUsbxchFlags_TxBusy      = 0x04, /**< IN endpoint 1 transmit in progress */
} FdUsbxchFlags;

/**
 * @brief USB Exchange (Vendor Class Bulk) driver handle.
 * @details Manages 2 x IN and 2 x OUT bulk endpoints with ring-buffered 
 *          streaming. Designed for high-throughput data exchange between 
 *          host and device (DAQ, firmware update, etc.).
 */
typedef struct {
    uint8_t flags;                  /**< State flags (FdUsbxchFlags) */
    /* --- TX buffers (device-to-host) --- */
    FdCRing tx;  /**< Ring buffer for EP1 transmit */
    size_t txToRelease;
    FdCRing rx;  /**< Ring buffer for EP1 receive */
    size_t rxToRelease;
} FdUsbxchHandle;

/* ---- Public API ---- */

{% set inst = usbxch %}
void FdUsbxch_{{inst.name}}_Init(FdUsbxchHandle *h);
uint32_t FdUsbxch_{{inst.name}}_Write(FdUsbxchHandle *h, const uint8_t* data, uint32_t length);
uint32_t FdUsbxch_{{inst.name}}_WriteAvailable(FdUsbxchHandle *h);
uint32_t FdUsbxch_{{inst.name}}_Flush(FdUsbxchHandle *h);
uint32_t FdUsbxch_{{inst.name}}_Clear(FdUsbxchHandle *h);
uint32_t FdUsbxch_{{inst.name}}_Read(FdUsbxchHandle *h, uint8_t *data, uint32_t length);
bool FdUsbxch_{{inst.name}}_ReadReserve(FdUsbxchHandle *h, void** readptr1, uint16_t* cont1, void** readptr2, uint16_t* cont2);
bool FdUsbxch_{{inst.name}}_ReadCommit(FdUsbxchHandle *h, size_t len);
uint32_t FdUsbxch_{{inst.name}}_ReadAvailable(FdUsbxchHandle *h);
bool FdUsbxch_{{inst.name}}_Connected(FdUsbxchHandle *h);
void FdUsbxch_{{inst.name}}_Update(FdUsbxchHandle *h);
extern FdUsbxchHandle {{inst.name}};


// ==========================================
// UNIFIED COMPILE-TIME DISPATCH MACROS
// ==========================================

#define FDUSBXCH_DISPATCH_IMPL(inst, suffix) FdUsbxch_##inst##suffix
#define FDUSBXCH_DISPATCH(inst, suffix) FDUSBXCH_DISPATCH_IMPL(inst, suffix)

#define FdUsbxch_Init(inst)                    FDUSBXCH_DISPATCH(inst, _Init)(&(inst))
#define FdUsbxch_Write(inst, ...)              FDUSBXCH_DISPATCH(inst, _Write)(&(inst), ##__VA_ARGS__)
#define FdUsbxch_WriteAvailable(inst)          FDUSBXCH_DISPATCH(inst, _WriteAvailable)(&(inst))
#define FdUsbxch_Flush(inst)                   FDUSBXCH_DISPATCH(inst, _Flush)(&(inst))
#define FdUsbxch_Clear(inst)                   FDUSBXCH_DISPATCH(inst, _Clear)(&(inst))
#define FdUsbxch_Read(inst, ...)               FDUSBXCH_DISPATCH(inst, _Read)(&(inst), ##__VA_ARGS__)
#define FdUsbxch_ReadReserve(inst, ...)        FDUSBXCH_DISPATCH(inst, _ReadReserve)(&(inst), ##__VA_ARGS__)
#define FdUsbxch_ReadCommit(inst, ...)         FDUSBXCH_DISPATCH(inst, _ReadCommit)(&(inst), ##__VA_ARGS__)
#define FdUsbxch_ReadAvailable(inst)           FDUSBXCH_DISPATCH(inst, _ReadAvailable)(&(inst))
#define FdUsbxch_Connected(inst)               FDUSBXCH_DISPATCH(inst, _Connected)(&(inst))
#define FdUsbxch_Update(inst)                  FDUSBXCH_DISPATCH(inst, _Update)(&(inst))


#ifdef __cplusplus
}
#endif
#endif