#pragma once
#include "Drv.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct{
} FdTimerHandle;


{% for inst in timer.instances %}
uint32_t FdTimer_{{inst.name}}_Init(FdTimerHandle *h, uint32_t freq);
uint32_t FdTimer_{{inst.name}}_Frequency(FdTimerHandle *h, uint32_t freq);
static inline uint32_t FdTimer_{{inst.name}}_PeriodNs(FdTimerHandle *h, uint32_t ns){if (ns == 0U) return 0;FdTimer_{{inst.name}}_Frequency(h, (1000000000/ns) );}
uint32_t FdTimer_{{inst.name}}_Enable(FdTimerHandle *h, uint8_t en);

__attribute__((weak)) void FdTimer_{{inst.name}}_PeriodCallback(void);
__attribute__((weak)) void FdTimer_{{inst.name}}_ChannelCallback(uint8_t channelNum);
extern FdTimerHandle {{inst.name}};
{% endfor %}


#ifdef __cplusplus
}
#endif

#define FDTIMER_DISPATCH_IMPL(inst, suffix) FdTimer_##inst##suffix
#define FDTIMER_DISPATCH(inst, suffix) FDTIMER_DISPATCH_IMPL(inst, suffix)

#define FdTimer_Init(inst, ...)       FDTIMER_DISPATCH(inst, _Init)(&(inst), ##__VA_ARGS__)
#define FdTimer_Frequency(inst, ...)  FDTIMER_DISPATCH(inst, _Frequency)(&(inst), ##__VA_ARGS__)
#define FdTimer_PeriodNs(inst, ...)   FDTIMER_DISPATCH(inst, _PeriodNs)(&(inst), ##__VA_ARGS__)
#define FdTimer_Enable(inst, ...)     FDTIMER_DISPATCH(inst, _Enable)(&(inst), ##__VA_ARGS__)