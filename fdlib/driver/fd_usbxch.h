#ifndef USBXCH
#define USBXCH
#include <stdint.h>
#include <stdbool.h>
#include "fd_ring.h"


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
    fd_ring_t tx;  /**< Ring buffer for EP1 transmit */
    size_t tx_to_release;
    fd_ring_t rx;  /**< Ring buffer for EP1 receive */
    size_t rx_to_release;
} fd_usbxch_handle_t;

/* ---- Public API ---- */

{% set inst = usbxch %}
void fd_usbxch_{{inst.name}}_init(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_{{inst.name}}_write(fd_usbxch_handle_t *h, const uint8_t* data, uint32_t length);
uint32_t fd_usbxch_{{inst.name}}_write_available(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_{{inst.name}}_flush(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_{{inst.name}}_clear(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_{{inst.name}}_read(fd_usbxch_handle_t *h, uint8_t *data, uint32_t length);
bool fd_usbxch_{{inst.name}}_read_reserve(fd_usbxch_handle_t *h, void** readptr1, uint16_t* cont1, void** readptr2, uint16_t* cont2);
bool fd_usbxch_{{inst.name}}_read_commit(fd_usbxch_handle_t *h, size_t len);
uint32_t fd_usbxch_{{inst.name}}_read_available(fd_usbxch_handle_t *h);
bool fd_usbxch_{{inst.name}}_connected(fd_usbxch_handle_t *h);
void fd_usbxch_{{inst.name}}_update(fd_usbxch_handle_t *h);
extern fd_usbxch_handle_t {{inst.name}};


// ==========================================
// UNIFIED COMPILE-TIME DISPATCH MACROS
// ==========================================

#define FD_USBXCH_DISPATCH_IMPL(inst, suffix) fd_usbxch_##inst##suffix
#define FD_USBXCH_DISPATCH(inst, suffix) FD_USBXCH_DISPATCH_IMPL(inst, suffix)

#define fd_usbxch_init(inst)                    FD_USBXCH_DISPATCH(inst, _init)(&(inst))
#define fd_usbxch_write(inst, ...)              FD_USBXCH_DISPATCH(inst, _write)(&(inst), ##__VA_ARGS__)
#define fd_usbxch_write_available(inst)         FD_USBXCH_DISPATCH(inst, _write_available)(&(inst))
#define fd_usbxch_flush(inst)                   FD_USBXCH_DISPATCH(inst, _flush)(&(inst))
#define fd_usbxch_clear(inst)                   FD_USBXCH_DISPATCH(inst, _clear)(&(inst))
#define fd_usbxch_read(inst, ...)               FD_USBXCH_DISPATCH(inst, _read)(&(inst), ##__VA_ARGS__)
#define fd_usbxch_read_reserve(inst, ...)       FD_USBXCH_DISPATCH(inst, _read_reserve)(&(inst), ##__VA_ARGS__)
#define fd_usbxch_read_commit(inst, ...)        FD_USBXCH_DISPATCH(inst, _read_commit)(&(inst), ##__VA_ARGS__)
#define fd_usbxch_read_available(inst)          FD_USBXCH_DISPATCH(inst, _read_available)(&(inst))
#define fd_usbxch_connected(inst)               FD_USBXCH_DISPATCH(inst, _connected)(&(inst))
#define fd_usbxch_update(inst)                  FD_USBXCH_DISPATCH(inst, _update)(&(inst))


#ifdef __cplusplus
}
#endif
#endif