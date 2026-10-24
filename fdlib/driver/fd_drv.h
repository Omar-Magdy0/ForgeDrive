#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

#define FD_DEVICE_{{device.name | upper}}
#define FD_PLATFORM_{{device.platform | upper}}
#define FD_FAMILY_{{device.family | upper}}


typedef struct {
    float adc_to_V;
    float adc_ref_V;
} fd_drv_t;

extern fd_drv_t DRV;

void fd_drv_init(fd_drv_t *h);
uint32_t fd_drv_tick(void);
uint32_t fd_drv_tock(uint32_t start);

typedef uint16_t fd_pin_t;

uint32_t fd_atomic_start(void);
void fd_atomic_end(uint32_t primask);
void fd_delay(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif