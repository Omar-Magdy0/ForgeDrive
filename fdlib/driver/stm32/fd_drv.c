


#include "fd_drv.h"
#include "stm32{{device.family}}xx.h"
#include "stm32{{device.family}}xx_hal.h"
#include "stm32{{device.family}}xx_ll_rcc.h"
#include "System.h"

// TODO: Add IRQ binding system to the DRV, rather in source IRQ definition and conflict, code gen resolves IRQ bind
#if defined(DWT) && defined(DWT_CTRL_CYCCNTENA_Msk) && \
    defined(CoreDebug_DEMCR_TRCENA_Msk)
#define FD_DWT_CYCCNT_SUPPORTED 1
#else
#define FD_DWT_CYCCNT_SUPPORTED 0 
#endif

static inline void dwt_init(void)
{
    #if FD_DWT_CYCCNT_SUPPORTED
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    #endif
}

fd_drv_t DRV;

#if FD_DWT_CYCCNT_SUPPORTED
uint32_t fd_drv_tick(void)
{
    return DWT->CYCCNT;
}

uint32_t fd_drv_tock(uint32_t start)
{
    return (DWT->CYCCNT - start);
}
#endif

uint32_t fd_atomic_start(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void fd_atomic_end(uint32_t primask)
{
    __set_PRIMASK(primask);
}

void fd_delay(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

void fd_drv_init(fd_drv_t *h)
{
    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    dwt_init();
    if (h != NULL)
    {
        h->adc_to_V = (3.3/4095.0);
    }
}