#ifndef USBXCH
#define USBXCH
#include <stdint.h>
#include <stdbool.h>
#include "fd/fd_ring.h"
#include "fd_driver_conf.h"


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

{% for inst in usbxch.instances %}
void fd_usbxch_{{inst.name}}_init(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_write_impl(fd_usbxch_handle_t *h, const uint8_t* data, uint32_t length);
uint32_t fd_usbxch_write_available_impl(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_flush_impl(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_clear_impl(fd_usbxch_handle_t *h);
uint32_t fd_usbxch_read_impl(fd_usbxch_handle_t *h, uint8_t *data, uint32_t length);
uint32_t fd_usbxch_read_available_impl(fd_usbxch_handle_t *h);
bool fd_usbxch_connected_impl(fd_usbxch_handle_t *h);
void fd_usbxch_update_impl(fd_usbxch_handle_t *h);
extern fd_usbxch_handle_t {{inst.name}};
{% endfor %}

// ==========================================
// UNIFIED COMPILE-TIME DISPATCH MACROS
// ==========================================

#define FD_USBXCH_DISPATCH_IMPL(inst, suffix) fd_usbxch_##inst##suffix
#define FD_USBXCH_DISPATCH(inst, suffix) FD_USBXCH_DISPATCH_IMPL(inst, suffix)

#define fd_usbxch_init(inst)                    FD_USBXCH_DISPATCH(inst, _init)(&(inst))
#define fd_usbxch_write(inst, data, len)        fd_usbxch_write_impl(&(inst), data, len)
#define fd_usbxch_write_available(inst)         fd_usbxch_write_available_impl(&(inst))
#define fd_usbxch_flush(inst)                   fd_usbxch_flush_impl(&(inst))
#define fd_usbxch_clear(inst)                   fd_usbxch_clear_impl(&(inst))
#define fd_usbxch_read(inst, data, len)         fd_usbxch_read_impl(&(inst), data, len)
#define fd_usbxch_read_available(inst)          fd_usbxch_read_available_impl(&(inst))
#define fd_usbxch_connected(inst)               fd_usbxch_connected_impl(&(inst))
#define fd_usbxch_update(inst)                  fd_usbxch_update_impl(&(inst))  

#ifdef __cplusplus
}
#endif
#endif