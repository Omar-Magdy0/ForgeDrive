#include "Timer.h"
#include "stm32{{device.family}}xx_ll_tim.h"
#include "stm32{{device.family}}xx_ll_gpio.h"
#include "System.h"
//TODO: add PWM, CAPTURE , PULSE support, etc in a neat yet simple way


{% for inst in timer.instances %}
uint32_t FdTimer_{{inst.name}}_Init(FdTimerHandle *h, uint32_t freq)
{
    __FDGEN__CLK_EN(TIM{{ inst.tim }})
    TIM_TypeDef * const tim = TIM{{inst.tim}};
    const uint32_t timClk = PeriphBusClk(tim);

    LL_TIM_InitTypeDef timInit = {
        .Prescaler         = 0,
        .CounterMode       = LL_TIM_COUNTERMODE_UP,
        .Autoreload        = __LL_TIM_CALC_ARR(timClk, 1, freq),
        .ClockDivision     = LL_TIM_CLOCKDIVISION_DIV1,
        .RepetitionCounter = 0
    };

    LL_TIM_DisableCounter(tim);
    LL_TIM_EnableARRPreload(tim);
    LL_TIM_SetUpdateSource(tim, LL_TIM_UPDATESOURCE_COUNTER);
    LL_TIM_Init(tim, &timInit);
    LL_TIM_GenerateEvent_UPDATE(tim);
    LL_TIM_ClearFlag_UPDATE(tim);
    //Timer configuration done, lets do the gpio and channels (TODO)
    LL_TIM_EnableIT_UPDATE(TIM1);
    __FDGEN__IRQ_EN(TIM{{ inst.tim }}, {{ inst.irq["tim"] }})
    __FDGEN__IRQ_BIND(TIM{{ inst.tim }}, {{inst.name}}_tim{{inst.tim}}_irqhandler)
    return 0;
}
uint32_t FdTimer_{{inst.name}}_Frequency(FdTimerHandle *h, uint32_t freq)
{
    TIM_TypeDef * const tim = TIM{{inst.tim}};
    const uint32_t timClk = PeriphBusClk(tim);
    const uint32_t arr = __LL_TIM_CALC_ARR(timClk, 1U, freq);
    LL_TIM_SetAutoReload(tim, arr);
    LL_TIM_GenerateEvent_UPDATE(tim);
    LL_TIM_ClearFlag_UPDATE(tim);
    return 0;
}
uint32_t FdTimer_{{ inst.name }}_Enable(FdTimerHandle *h, uint8_t en)
{
    TIM_TypeDef * const tim = TIM{{ inst.tim }};
    if (en) {
        LL_TIM_EnableCounter(tim);
    } else {
        LL_TIM_DisableCounter(tim);
    }
    return 0;
}
__attribute__((weak)) void FdTimer_{{inst.name}}_PeriodCallback(void){}
__attribute__((weak)) void FdTimer_{{inst.name}}_ChannelCallback(uint8_t channel_num){}
FdTimerHandle {{inst.name}};

void {{inst.name}}_tim{{inst.tim}}_irqhandler()
{
    TIM_TypeDef *tim = TIM{{inst.tim}};
    if (LL_TIM_IsActiveFlag_UPDATE(tim)) {
        LL_TIM_ClearFlag_UPDATE(tim);
        FdTimer_{{inst.name}}_PeriodCallback();
    }
    //This should be generated/unrolled based on static time config
    if (LL_TIM_IsActiveFlag_CC1(tim)){FdTimer_{{inst.name}}_ChannelCallback(1);}
    if (LL_TIM_IsActiveFlag_CC2(tim)){FdTimer_{{inst.name}}_ChannelCallback(2);}
    if (LL_TIM_IsActiveFlag_CC3(tim)){FdTimer_{{inst.name}}_ChannelCallback(3);}
    if (LL_TIM_IsActiveFlag_CC4(tim)){FdTimer_{{inst.name}}_ChannelCallback(4);}
}


{% endfor %}