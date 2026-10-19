#include "cortos.h"
#include "stm32f1xx.h"
#include "cortos_conf.h"

uint32_t CORTOS_PortProfTick_rate_hz(){return SystemCoreClock;} 
void CORTOS_PortTickerInit(){SysTick_Config(SystemCoreClock / cortosTICK_RATE_HZ);}
void CORTOS_PortProfTickerInit()
{//USE DWT for profiling in freerunning mode
    // Enable TRC (trace)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Reset cycle counter
    DWT->CYCCNT = 0;

    // Enable cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void CORTOS_PortIdle() { __WFI(); }
void CORTOS_PortWake(void){}

uint32_t CORTOS_PortEnterCritical(void)
{
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

void CORTOS_PortExitCritical(uint32_t state)
{
    __set_PRIMASK(state);
}

uint32_t CORTOS_PortProfTicks() { return DWT->CYCCNT; };


//Systick IRQ
extern void Ticker_IRQ();
void SysTick_Handler(void)
{
    Ticker_IRQ();
    HAL_IncTick();
}