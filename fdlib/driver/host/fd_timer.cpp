#include "fd_timer.h"
#include "fd_SILTick.h"
#include "stdint.h"


{% for inst in timer.instances %}
void {{inst.name}}_periodCB();
bool {{inst.name}}_enabled = false;
uint64_t {{inst.name}}_freq = 1;

uint32_t fd_timer_{{inst.name}}_init(fd_tim_handle_t *h, uint32_t freq)
{
    {{inst.name}}_freq = freq;
    return 0;
}
uint32_t fd_timer_{{inst.name}}_frequency(fd_tim_handle_t *h, uint32_t freq)
{
    {{inst.name}}_freq = freq;
    return 0;
}
uint32_t fd_timer_{{inst.name}}_enable(fd_tim_handle_t *h, uint8_t en)
{
    {{inst.name}}_enabled = en;
    if(en)SILTick::instance().register_event(1'000'000'000/{{inst.name}}_freq, {{inst.name}}_periodCB);
    return 0;
}

__attribute__((weak)) void fd_timer_{{inst.name}}_periodCallback(void){}
__attribute__((weak)) void fd_timer_{{inst.name}}_channelCallback(uint8_t channel_num){}
fd_tim_handle_t {{inst.name}};

void {{inst.name}}_periodCB()
{
    if({{inst.name}}_enabled){
        SILTick::instance().register_event(1'000'000'000/{{inst.name}}_freq, {{inst.name}}_periodCB);
    }
    fd_timer_{{inst.name}}_periodCallback();
}

{% endfor %}
