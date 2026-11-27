/**
 * @file    fd_usbxch.c
 * @author  Carol Nasser
 * @brief   USB Vendor Class Bulk Exchange Driver for STM32F4.
 * @details Implements a dual-bulk-pair (2×IN + 2×OUT) USB transfer driver
 *          using the STM32 USB Device library (LL + HAL PCD layer). Data is
 *          buffered through `FdCRing` ring buffers and transferred
 *          asynchronously via the USB ISR callbacks.
 *
 *          Endpoint map:
 *            EP0  – Control (standard)
 *            EP1  – Bulk OUT 0x01 / Bulk IN 0x81
 *
 *          The USB Device stack (USBD_HandleTypeDef) and the lower-level PCD
 *          handle (hpcd_USB_OTG_FS) are expected to be provided by the
 *          STM32CubeMX-generated middleware.
 */

#include "Usbxch.h"    
#include "System.h"
#include "Drv.h"
#include <string.h>
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_xch.h"



/* ===========================================================================
 * Public API
 * =========================================================================== */
static USBD_HandleTypeDef hUsbDeviceFS;
static void Usbxch_StartTx(FdUsbxchHandle *h);
extern PCD_HandleTypeDef hpcd;
static USBD_XCH_HandleTypeDef s_usbxch;

{% set inst = usbxch %}
/* ---------------------------------------------------------------------------
 * Internal forward declarations
 * --------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------
 * Handle singleton for callback dispatch
 * ---------------------------------------------------------------------------
 * The USB stack callbacks (received from PCD / USBD layer) carry only the
 * endpoint address; we need a way to reach the handle.  Because the driver
 * is typically used as a single instance, we keep a static pointer.
 * For multi-instance support, extend with a lookup table keyed by itf.
 */
void {{inst.name}}_DeviceInit()
{
    if (USBD_Init(&hUsbDeviceFS, &XCH_Desc, DEVICE_FS) != USBD_OK)
    {
      Error_Handler();
    }
    // Class and interface registration
    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_XCH) != USBD_OK)
    {
      Error_Handler();
    }
    if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
    {
      Error_Handler();
    }
}

uint8_t {{inst.name}}_txBuf[{{inst.tx_bufsize}}];
uint8_t {{inst.name}}_rxBuf[{{inst.rx_bufsize}}];
FdUsbxchHandle {{inst.name}};

void FdUsbxch_{{inst.name}}_Init(FdUsbxchHandle *h)
{
    /* --- Clear the handle --- */
    memset(h, 0, sizeof(*h));
    /* --- Initialise ring buffers --- */
    FdCRing_Init(&h->tx, {{inst.name}}_txBuf, sizeof({{inst.name}}_txBuf));
    FdCRing_Init(&h->rx, {{inst.name}}_rxBuf, sizeof({{inst.name}}_rxBuf));
    s_usbxch.ctx = h;
    hUsbDeviceFS.pClassData = (void*)&s_usbxch;
    {{inst.name}}_DeviceInit();
}

/* ---------------------------------------------------------------------------
 * Write
 * --------------------------------------------------------------------------- */
uint32_t FdUsbxch_{{inst.name}}_Write(FdUsbxchHandle *h, const uint8_t *data, uint32_t length)
{
    void *w1, *w2;
    uint16_t c1, c2;
    if (FdCRing_WriteReserve(&h->tx, (uint16_t)length, &w1, &c1, &w2, &c2)) {
        memcpy(w1, data, c1);
        if(w2)memcpy(w2, data + c1, c2);
        FdCRing_WriteCommit(&h->tx, length);
        return length;
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * Write available (space left in tx buffer)
 * --------------------------------------------------------------------------- */
uint32_t FdUsbxch_{{inst.name}}_WriteAvailable(FdUsbxchHandle *h)
{
    return (uint32_t)FdCRing_Free(&h->tx);
}

/* ---------------------------------------------------------------------------
 * Flush – trigger an IN transfer immediately if data is pending
 * --------------------------------------------------------------------------- */
uint32_t FdUsbxch_{{inst.name}}_Flush(FdUsbxchHandle *h)
{
    if (h->flags & FdUsbxchFlags_TxBusy)
        return 1; /* already in progress */

    Usbxch_StartTx(h);
    return 0;
}


/* ---------------------------------------------------------------------------
 * Clear – drain all ring buffers and reset stats
 * --------------------------------------------------------------------------- */
uint32_t FdUsbxch_{{inst.name}}_Clear(FdUsbxchHandle *h)
{
    /* Drain TX: reset head/tail to empty and clear any stale transfer state */
    h->tx.head = h->tx.tail = 0;
    h->rx.head = h->rx.tail = 0;
    h->txToRelease = 0;
    h->rxToRelease = 0;
    h->flags &= ~FdUsbxchFlags_TxBusy;
    return 0;
}

bool FdUsbxch_{{inst.name}}_ReadReserve(FdUsbxchHandle *h, void** readptr1, uint16_t* cont1, void** readptr2, uint16_t* cont2)
{
    return FdCRing_ReadReserve(&h->rx, readptr1, cont1, readptr2, cont2);
}

bool FdUsbxch_{{inst.name}}_ReadCommit(FdUsbxchHandle *h, size_t len)
{
    if (len > FdCRing_Size(&h->rx))
        return false;

    FdCRing_ReadCommit(&h->rx, (uint16_t)len);
    return true;
}
/* ---------------------------------------------------------------------------
 * Read from OUT endpoint 1
 * --------------------------------------------------------------------------- */
uint32_t FdUsbxch_{{inst.name}}_Read(FdUsbxchHandle *h, uint8_t *data, uint32_t length)
{
    void *r1, *r2;
    uint16_t c1, c2;
    uint32_t total = 0;
    if (FdCRing_ReadReserve(&h->rx, &r1, &c1, &r2, &c2)) {
        uint16_t to_copy = (uint16_t)((length < (uint32_t)(c1 + c2)) ? length : (uint32_t)(c1 + c2));

        uint16_t copy_from_first = (to_copy < c1) ? to_copy : c1;
        memcpy(data, r1, copy_from_first);
        total += copy_from_first;

        if (total < length && r2) {
            uint16_t rem = (uint16_t)(length - total);
            uint16_t copy_from_second = (rem < c2) ? rem : c2;
            memcpy(data + copy_from_first, r2, copy_from_second);
            total += copy_from_second;
        }
        FdCRing_ReadCommit(&h->rx, (uint16_t)total);
    }
    return total;
}

/* ---------------------------------------------------------------------------
 * Read available (occupied bytes in RX ring)
 * --------------------------------------------------------------------------- */
uint32_t FdUsbxch_{{inst.name}}_ReadAvailable(FdUsbxchHandle *h)
{
    uint16_t occ = FdCRing_Size(&h->rx);
    return occ;
}

/* ---------------------------------------------------------------------------
 * Connected status – true when VBUS present and SET_CONFIGURATION received
 * --------------------------------------------------------------------------- */
bool FdUsbxch_{{inst.name}}_Connected(FdUsbxchHandle *h)
{
    return (h->flags & FdUsbxchFlags_Configured) != 0;
}

/* ---------------------------------------------------------------------------
 * Update – periodic housekeeping (call from main/super-loop)
 * --------------------------------------------------------------------------- */
void FdUsbxch_{{inst.name}}_Update(FdUsbxchHandle *h)
{
    /* Start IN transfer for EP1 if data is pending */
    if (!(h->flags & FdUsbxchFlags_TxBusy) && FdCRing_Size(&h->tx) > 0)
        Usbxch_StartTx(h);
}

/* ===========================================================================
 * Internal helpers
 * =========================================================================== */

/**
 * @brief  Kick-start an IN (device-to-host) transfer on the given endpoint.
 * @note   Called from thread context (update / write) – safe with IRQs enabled.
 *         The actual HAL_PCD_EP_Transmit() can be called with IRQs enabled;
 *         it will raise an interrupt on completion.
 */
static void Usbxch_StartTx(FdUsbxchHandle *h)
{
    FdCRing *tx;
    uint8_t  busy_flag;

    tx       = &h->tx;
    busy_flag = FdUsbxchFlags_TxBusy;

    void *w1, *w2;
    uint16_t c1, c2;
    if (h->flags & busy_flag) {
        return;
    }

    if (!FdCRing_ReadReserve(tx, &w1, &c1, &w2, &c2)) {
        /* Nothing to send */
        return;
    }
    if (USBD_XCH_Transmit(&hUsbDeviceFS, w1, c1) != USBD_OK) {
        h->flags &= ~busy_flag;
        return;
    }
    h->txToRelease = c1;
    h->flags |= busy_flag;
}


void USBD_XCH_GetRxStorage(void *ctx, uint8_t **ptr, uint16_t *size)
{
    FdUsbxchHandle *h = ctx;
    void *r1, *r2;
    uint16_t c1, c2;
    uint16_t fs = FdCRing_Free(&h->rx);

    if (fs == 0U) {
        *ptr = NULL;
        *size = 0U;
        return;
    }

    if (!FdCRing_WriteReserve(&h->rx, fs, &r1, &c1, &r2, &c2)) {
        *ptr = NULL;
        *size = 0U;
        return;
    }

    *ptr = r1;
    *size = c1;
}

void USBD_XCH_RxComplete(void *ctx, uint8_t *rx_buf, uint16_t size)
{
    FdUsbxchHandle *h = ctx;
    FdCRing_WriteCommit(&h->rx, size);
}

void USBD_XCH_TxComplete(void *ctx)
{
    FdUsbxchHandle *h = ctx;
    FdCRing_ReadCommit(&h->tx, h->txToRelease);

    void *r1, *r2;
    uint16_t c1, c2;
    if (!FdCRing_ReadReserve(&h->tx, &r1, &c1, &r2, &c2)) {
        /* Nothing to send */
        h->flags &= ~FdUsbxchFlags_TxBusy;
        return;
    }
    if (USBD_XCH_Transmit(&hUsbDeviceFS, r1, c1) != USBD_OK) {
        h->flags &= ~FdUsbxchFlags_TxBusy;
        return;
    }
    h->txToRelease = c1;
    h->flags |= FdUsbxchFlags_TxBusy;
}

