


#include "Drv.h"
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

FdDrv DRV;

void FdDrv_DwtInit(void)
{
    #if FD_DWT_CYCCNT_SUPPORTED
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    #endif
}

uint32_t FdDrv_DwtTick(void)
{
    #if FD_DWT_CYCCNT_SUPPORTED
    return DWT->CYCCNT;
    #endif
}

uint32_t FdDrv_AtomicStart(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void FdDrv_AtomicEnd(uint32_t primask)
{
    __set_PRIMASK(primask);
}

void FdDrv_Delay(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

void FdDrv_Init(FdDrv *h)
{
    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    SystemPeriphClock_Init();
    SystemNVIC_Init();
    SystemGPIO_Init();
    if (h != NULL)
    {
        h->adc2Vlt = (3.3/4095.0);
    }
}