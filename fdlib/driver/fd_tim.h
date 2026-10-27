#pragma once
#include "fd_drv.h"
#include <stdint.h>


typedef struct{
} fd_tim_handle_t;


{% for inst in tim.instances %}
uint32_t fd_tim_{{inst.name}}_init(fd_tim_handle_t *h, uint32_t freq);
uint32_t fd_tim_{{inst.name}}_frequency(fd_tim_handle_t *h, uint32_t freq);
static inline uint32_t fd_tim_{{inst.name}}_period_ns(fd_tim_handle_t *h, uint32_t ns){if (ns == 0U) return 0;fd_tim_{{inst.name}}_frequency(h, (1000000000/ns) );}
uint32_t fd_tim_{{inst.name}}_enable(fd_tim_handle_t *h, uint8_t en);

__attribute__((weak)) void fd_tim_{{inst.name}}_periodCallback(void);
__attribute__((weak)) void fd_tim_{{inst.name}}_channelCallback(uint8_t channel_num);
extern fd_tim_handle_t {{inst.name}};
{% endfor %}


