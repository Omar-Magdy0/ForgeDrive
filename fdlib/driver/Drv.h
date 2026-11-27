#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif


#define FDDRV_DEVICE_{{device.name | upper}}
#define FDDRV_PLATFORM_{{device.platform | upper}}
#define FDDRV_FAMILY_{{device.family | upper}}
#define FDDRV_DEVICE_CPUCORE_{{device.cpu_core | upper}}

#define __FDGEN__IRQ_BIND(irq_name, handler)
#define __FDGEN__IRQ_EN(irq_name, prio)
#define __FDGEN__CLK_EN(periph)

#define FDDRV_ERR_INVALID_ARG 1
#define FDDRV_ERR_RANGE 2

typedef struct {
    float adc2Vlt;
    float adcRefVlt;
} FdDrv;

extern FdDrv DRV;

void FdDrv_Init(FdDrv *h);
uint32_t FdDrv_AtomicStart(void);
void FdDrv_AtomicEnd(uint32_t primask);
void FdDrv_Delay(uint32_t delay_ms);
void FdDrv_DwtInit(void);
uint32_t FdDrv_DwtTick(void);


#ifdef __cplusplus
}
#endif