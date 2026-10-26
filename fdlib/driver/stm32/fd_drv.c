


#include "fd_drv.h"
#include "stm32{{device.family}}xx.h"
#include "stm32{{device.family}}xx_hal.h"
#include "stm32{{device.family}}xx_ll_rcc.h"
#include "System.h"

#if defined(DWT) && defined(DWT_CTRL_CYCCNTENA_Msk) && \
    defined(CoreDebug_DEMCR_TRCENA_Msk)
#define FD_DWT_CYCCNT_SUPPORTED 1
#else
#define FD_DWT_CYCCNT_SUPPORTED 0 
#endif

fd_drv_t DRV;

void fd_dwt_init(void)
{
    #if FD_DWT_CYCCNT_SUPPORTED
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    #endif
}

uint32_t fd_dwt_tick(void)
{
    #if FD_DWT_CYCCNT_SUPPORTED
    return DWT->CYCCNT;
    #endif
}

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
    SystemPeriphClock_Init();
    SystemNVIC_Init();
    if (h != NULL)
    {
        h->adc_to_V = (3.3/4095.0);
    }
}