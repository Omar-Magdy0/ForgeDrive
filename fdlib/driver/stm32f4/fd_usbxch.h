#ifndef USBXCH
#define USBXCH
#include <stdint.h>
#include <stdbool.h>
#include "fd/fd_ring.h"
#include "fd_driver_conf.h"
#include "usbd_xch.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief USB Exchange handle state flags. */
typedef enum {
    USBXCH_FLAG_NONE        = 0x00,
    USBXCH_FLAG_CONFIGURED  = 0x01, /**< Bus is configured (SET_CONFIGURATION received) */
    USBXCH_FLAG_SUSPENDED   = 0x02, /**< Bus is suspended */
    USBXCH_FLAG_TX_BUSY    = 0x04, /**< IN endpoint 1 transmit in progress */
} fd_usbxch_flags_t;

/**
 * @brief USB Exchange (Vendor Class Bulk) driver handle.
 * @details Manages 2 x IN and 2 x OUT bulk endpoints with ring-buffered 
 *          streaming. Designed for high-throughput data exchange between 
 *          host and device (DAQ, firmware update, etc.).
 */
typedef struct {
    uint8_t flags;                  /**< State flags (fd_usbxch_flags_t) */
    /* --- TX buffers (device-to-host) --- */
    uint8_t tx_storage[FD_USBXCH_TX_BUFFSIZE]; /**< IN EP1 backing storage */
    fd_ring_t tx;  /**< Ring buffer for EP1 transmit */
    uint16_t tx_to_release;
    /* --- RX buffers (host-to-device) --- */
    uint8_t rx_storage[FD_USBXCH_RX_BUFFSIZE]; /**< OUT EP1 backing storage */
    void *r1;
    uint16_t c1;
    fd_ring_t rx;  /**< Ring buffer for EP1 receive */
    USBD_XCH_HandleTypeDef usbxch;
} fd_usbxch_handle_t;

/* ---- Public API ---- */

/**
 * @brief  Initialises the USBXCH driver, configures I/O and USB device stack.
 * @param  h: Handle instance (must point to valid memory, zeroed on entry).
 */
void fd_usbxch_init(fd_usbxch_handle_t *h);

/**
 * @brief  Writes data to IN endpoint 1 (device-to-host, bulk EP 0x81).
 * @param  h: Handle.
 * @param  data: Source buffer.
 * @param  length: Number of bytes to write.
 * @retval uint32_t: Number of bytes actually enqueued.
 */
uint32_t fd_usbxch_write(fd_usbxch_handle_t *h, const uint8_t* data, uint32_t length);

/**
 * @brief  Returns number of bytes that can be written to EP1 without blocking.
 * @param  h: Handle.
 * @return Available free space (bytes) in EP1 TX ring buffer.
 */
uint32_t fd_usbxch_write_available(fd_usbxch_handle_t *h);

/**
 * @brief  Flushes (immediately starts) any pending USB IN transfer on EP1.
 * @param  h: Handle.
 * @retval 0 on success, non-zero if a transfer is already in progress.
 */
uint32_t fd_usbxch_flush(fd_usbxch_handle_t *h);

/**
 * @brief  Discards all data in both TX and both RX ring buffers.
 * @param  h: Handle.
 * @retval 1 always (for consistency with existing stub).
 */
uint32_t fd_usbxch_clear(fd_usbxch_handle_t *h);

/**
 * @brief  Reads received data from OUT endpoint 1 (host-to-device, bulk EP 0x01).
 * @param  h: Handle.
 * @param  data: Destination buffer.
 * @param  length: Maximum bytes to read.
 * @retval uint32_t: Actual bytes copied into @p data.
 */
uint32_t fd_usbxch_read(fd_usbxch_handle_t *h, uint8_t *data, uint32_t length);

/**
 * @brief  Returns number of bytes available to read from OUT EP1.
 * @param  h: Handle.
 * @return Occupied bytes in EP1 RX ring buffer.
 */
uint32_t fd_usbxch_read_available(fd_usbxch_handle_t *h);
/**
 * @brief  Checks whether the device is connected and configured.
 * @param  h: Handle.
 * @retval true  Device is connected and configured.
 * @retval false No VBUS or not yet configured by host.
 */
bool fd_usbxch_connected(fd_usbxch_handle_t *h);

/**
 * @brief  Performs periodic housekeeping (call from main loop):
 *         - Starts pending IN transfers if the bus is idle.
 *         - Prepares OUT endpoints for next reception.
 * @param  h: Handle.
 */
void fd_usbxch_update(fd_usbxch_handle_t *h);


#ifdef __cplusplus
}
#endif
#endif