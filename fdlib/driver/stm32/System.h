#pragma once
#include <stdint.h>
#include "stm32{{device.family}}xx_ll_rcc.h"


static inline uint32_t get_periph_bus_clk(const void *periph)
{
    {% for p in device.periph %}
    {% if p.name.startswith("TIM") and p.bus in device.bus_psc_getter %}
    if (periph == {{ p.name }}) {
        uint32_t clk = {{ device.bus_clk_getter[p.bus] }}();
        return ({{ device.bus_psc_getter[p.bus][0] }}() == {{ device.bus_psc_getter[p.bus][1] }})
            ? clk : 2U * clk;
    }
    {% else %}
    if (periph == {{ p.name }}) return {{ device.bus_clk_getter[p.bus] }}();
    {% endif %}
    {% endfor %}
    return 0U;
}


#define FD_CPU_CLK SystemCoreClock

void Error_Handler(void);
void SystemClock_Config();
void SystemPeriphClock_Init();
void SystemNVIC_Init();
void SystemGPIO_Init();