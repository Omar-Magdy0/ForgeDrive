#pragma once
#include "fd_drv.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct{
} fd_tim_handle_t;


{% for inst in timer.instances %}
uint32_t fd_timer_{{inst.name}}_init(fd_tim_handle_t *h, uint32_t freq);
uint32_t fd_timer_{{inst.name}}_frequency(fd_tim_handle_t *h, uint32_t freq);
static inline uint32_t fd_timer_{{inst.name}}_period_ns(fd_tim_handle_t *h, uint32_t ns){if (ns == 0U) return 0;fd_timer_{{inst.name}}_frequency(h, (1000000000/ns) );}
uint32_t fd_timer_{{inst.name}}_enable(fd_tim_handle_t *h, uint8_t en);

__attribute__((weak)) void fd_timer_{{inst.name}}_periodCallback(void);
__attribute__((weak)) void fd_timer_{{inst.name}}_channelCallback(uint8_t channel_num);
extern fd_tim_handle_t {{inst.name}};
{% endfor %}


#ifdef __cplusplus
}
#endif

#define FD_TIMER_DISPATCH_IMPL(inst, suffix) fd_timer_##inst##suffix
#define FD_TIMER_DISPATCH(inst, suffix) FD_TIMER_DISPATCH_IMPL(inst, suffix)

#define fd_timer_init(inst, ...)       FD_TIMER_DISPATCH(inst, _init)(&(inst), ##__VA_ARGS__)
#define fd_timer_frequency(inst, ...)  FD_TIMER_DISPATCH(inst, _frequency)(&(inst), ##__VA_ARGS__)
#define fd_timer_period_ns(inst, ...) FD_TIMER_DISPATCH(inst, _period_ns)(&(inst), ##__VA_ARGS__)
#define fd_timer_enable(inst, ...)     FD_TIMER_DISPATCH(inst, _enable)(&(inst), ##__VA_ARGS__)