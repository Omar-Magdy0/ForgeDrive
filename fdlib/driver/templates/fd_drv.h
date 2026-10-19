#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    int16_t adc_to_uV;
} fd_drv_t;

extern fd_drv_t fd_drv;

void fd_core_init(fd_drv_t *h);

uint32_t fd_prof_tick(void);
uint32_t fd_prof_tock(uint32_t start);

typedef uint16_t fd_pin_t;

typedef enum {
    FD_GPIO_MODE_INPUT = 0,
    FD_GPIO_MODE_OUTPUT,
    FD_GPIO_MODE_ALT,
    FD_GPIO_MODE_ANALOG
} fd_gpio_mode_t;

typedef enum {
    FD_GPIO_LOW = 0,
    FD_GPIO_HIGH = 1
} fd_gpio_state_t;

void fd_gpio_mode(fd_pin_t pin_num, fd_gpio_mode_t mode);
void fd_gpio_write(fd_pin_t pin_num, fd_gpio_state_t state);
void fd_gpio_toggle(fd_pin_t pin_num);
fd_gpio_state_t fd_gpio_read(fd_pin_t pin_num);

uint32_t fd_atomic_start(void);
void fd_atomic_end(uint32_t primask);

void fd_delay(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif