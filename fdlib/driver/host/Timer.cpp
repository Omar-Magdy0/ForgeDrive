#include "Timer.h"
#include "SILTick.h"
#include "stdint.h"


{% for inst in timer.instances %}
void {{inst.name}}_periodCB();
bool {{inst.name}}_enabled = false;
uint64_t {{inst.name}}_freq = 1;

uint32_t FdTimer_{{inst.name}}_Init(FdTimerHandle *h, uint32_t freq)
{
    {{inst.name}}_freq = freq;
    return 0;
}
uint32_t FdTimer_{{inst.name}}_Frequency(FdTimerHandle *h, uint32_t freq)
{
    {{inst.name}}_freq = freq;
    return 0;
}
uint32_t FdTimer_{{inst.name}}_Enable(FdTimerHandle *h, uint8_t en)
{
    {{inst.name}}_enabled = en;
    if(en)SILTick::Instance().RegisterEvent(1'000'000'000/{{inst.name}}_freq, {{inst.name}}_periodCB);
    return 0;
}

__attribute__((weak)) void FdTimer_{{inst.name}}_PeriodCallback(void){}
__attribute__((weak)) void FdTimer_{{inst.name}}_ChannelCallback(uint8_t channel_num){}
FdTimerHandle {{inst.name}};

void {{inst.name}}_periodCB()
{
    if({{inst.name}}_enabled){
        SILTick::Instance().RegisterEvent(1'000'000'000/{{inst.name}}_freq, {{inst.name}}_periodCB);
    }
    FdTimer_{{inst.name}}_PeriodCallback();
}

{% endfor %}
