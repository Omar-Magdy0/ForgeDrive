#include "fd_timer.h"
#include "stm32{{device.family}}xx_ll_tim.h"
#include "stm32{{device.family}}xx_ll_gpio.h"
#include "System.h"
//TODO: add PWM, CAPTURE , PULSE support, etc in a neat yet simple way


{% for inst in timer.instances %}
uint32_t fd_tim_{{inst.name}}_init(fd_tim_handle_t *h, uint32_t freq)
{
    __FDGEN__CLK_EN(TIM{{ inst.tim }})
    TIM_TypeDef * const tim = TIM{{inst.tim}};
    const uint32_t tim_clk = get_periph_bus_clk(tim);

    LL_TIM_InitTypeDef timInit = {
        .Prescaler         = 0,
        .CounterMode       = LL_TIM_COUNTERMODE_UP,
        .Autoreload        = __LL_TIM_CALC_ARR(tim_clk, 1, freq),
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
uint32_t fd_tim_{{inst.name}}_frequency(fd_tim_handle_t *h, uint32_t freq)
{
    TIM_TypeDef * const tim = TIM{{inst.tim}};
    const uint32_t tim_clk = get_periph_bus_clk(tim);
    const uint32_t arr = __LL_TIM_CALC_ARR(tim_clk, 1U, freq);
    LL_TIM_SetAutoReload(tim, arr);
    LL_TIM_GenerateEvent_UPDATE(tim);
    LL_TIM_ClearFlag_UPDATE(tim);
    return 0;
}
uint32_t fd_tim_{{ inst.name }}_enable(fd_tim_handle_t *h, uint8_t en)
{
    TIM_TypeDef * const tim = TIM{{ inst.tim }};
    if (en) {
        LL_TIM_EnableCounter(tim);
    } else {
        LL_TIM_DisableCounter(tim);
    }
    return 0;
}
__attribute__((weak)) void fd_tim_{{inst.name}}_periodCallback(void){}
__attribute__((weak)) void fd_tim_{{inst.name}}_channelCallback(uint8_t channel_num){}
fd_tim_handle_t {{inst.name}};

void {{inst.name}}_tim{{inst.tim}}_irqhandler()
{
    TIM_TypeDef *tim = TIM{{inst.tim}};
    if (LL_TIM_IsActiveFlag_UPDATE(tim)) {
        LL_TIM_ClearFlag_UPDATE(tim);
        fd_tim_{{inst.name}}_periodCallback();
    }
    //This should be generated/unrolled based on static time config
    if (LL_TIM_IsActiveFlag_CC1(tim)){fd_tim_{{inst.name}}_channelCallback(1);}
    if (LL_TIM_IsActiveFlag_CC2(tim)){fd_tim_{{inst.name}}_channelCallback(2);}
    if (LL_TIM_IsActiveFlag_CC3(tim)){fd_tim_{{inst.name}}_channelCallback(3);}
    if (LL_TIM_IsActiveFlag_CC4(tim)){fd_tim_{{inst.name}}_channelCallback(4);}
}


{% endfor %}