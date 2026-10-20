

#include "stm32f4xx.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_hal.h"
#include "fd_drv.h"

fd_drv_t fd_drv;

uint32_t fd_prof_tick(void)
{
    return DWT->CYCCNT;
}

uint32_t fd_prof_tock(uint32_t start)
{
    return (DWT->CYCCNT - start);
}

#ifndef GPIOF 
#define GPIOF 0
#endif
#ifndef GPIOG 
#define GPIOG 0
#endif
#ifndef GPIOH 
#define GPIOH 0
#endif

static GPIO_TypeDef* get_gpio_port(fd_pin_t n)
{
    if (n < 16)  return GPIOA;
    if (n < 32)  return GPIOB;
    if (n < 48)  return GPIOC;
    if (n < 64)  return GPIOD;
    if (n < 80)  return GPIOE;
    if (n < 96)  return (GPIO_TypeDef*)GPIOF;
    if (n < 112) return (GPIO_TypeDef*)GPIOG;
    return (GPIO_TypeDef*)GPIOH;
}

static uint32_t get_gpio_pin(fd_pin_t n)
{
    return (1U << (n % 16));
}

void fd_gpio_mode(fd_pin_t pin_num, fd_gpio_mode_t mode)
{
    uint32_t ll_mode;
    switch(mode) {
        case FD_GPIO_MODE_OUTPUT: ll_mode = LL_GPIO_MODE_OUTPUT; break;
        case FD_GPIO_MODE_ALT:    ll_mode = LL_GPIO_MODE_ALTERNATE; break;
        case FD_GPIO_MODE_ANALOG: ll_mode = LL_GPIO_MODE_ANALOG; break;
        case FD_GPIO_MODE_INPUT:  
        default:                  ll_mode = LL_GPIO_MODE_INPUT; break;
    }
    LL_GPIO_SetPinMode(get_gpio_port(pin_num), get_gpio_pin(pin_num), ll_mode);
}

void fd_gpio_write(fd_pin_t pin_num, fd_gpio_state_t state)
{
    if (state == FD_GPIO_HIGH) {
        LL_GPIO_SetOutputPin(get_gpio_port(pin_num), get_gpio_pin(pin_num));
    } else {
        LL_GPIO_ResetOutputPin(get_gpio_port(pin_num), get_gpio_pin(pin_num));
    }
}

void fd_gpio_toggle(fd_pin_t pin_num)
{
    LL_GPIO_TogglePin(get_gpio_port(pin_num), get_gpio_pin(pin_num));
}

fd_gpio_state_t fd_gpio_read(fd_pin_t pin_num)
{
    return (fd_gpio_state_t)LL_GPIO_IsInputPinSet(get_gpio_port(pin_num), get_gpio_pin(pin_num));
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

static inline void dwt_init(void)
{
    // Enable TRC (trace)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Reset cycle counter
    DWT->CYCCNT = 0;

    // Enable cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void fd_core_init(fd_drv_t *h)
{
    dwt_init();
    if (h != NULL) {
        h->adc_to_uV = 805;
    }
}