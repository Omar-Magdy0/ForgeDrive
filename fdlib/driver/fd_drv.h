#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif


#define FD_DEVICE_{{device.name | upper}}
#define FD_PLATFORM_{{device.platform | upper}}
#define FD_FAMILY_{{device.family | upper}}
#define FD_DEVICE_CPUCORE_{{device.cpu_core | upper}}

#define __FDGEN__IRQ_BIND(irq_name, handler)
#define __FDGEN__IRQ_EN(irq_name, prio)
#define __FDGEN__CLK_EN(periph)

#define FD_ERR_INVALID_ARG 1
#define FD_ERR_RANGE 1

typedef struct {
    float adc_to_V;
    float adc_ref_V;
} fd_drv_t;

extern fd_drv_t DRV;

void fd_drv_init(fd_drv_t *h);
typedef uint16_t fd_pin_t;

uint32_t fd_atomic_start(void);
void fd_atomic_end(uint32_t primask);
void fd_delay(uint32_t delay_ms);
void fd_dwt_init(void);
uint32_t fd_dwt_tick(void);


#ifdef __cplusplus
}
#endif