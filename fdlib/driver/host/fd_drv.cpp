#include "fd_drv.h"
#include <pthread.h>
#include <stdint.h>

pthread_mutex_t fd_atomic_mutex;
fd_drv_t fd_drv;
void fd_core_init(fd_drv_t *h)
{

}


uint32_t fd_prof_tick()
{
    return 0;
};

uint32_t fd_prof_tock(uint32_t start)
{
    (void)start;
    return 0;
};


/* Static Inline Implementation */

/**
 * @brief Sets the pin mode for a specific global pin number.
 */
void fd_gpio_mode(fd_pin_t pin_num, fd_gpio_mode_t mode) {

}

/**
 * @brief Writes to a pin using the BSRR register (Atomic).
 * If state is HIGH, it writes to the lower 16 bits; if LOW, the upper 16 bits.
 */
void fd_gpio_write(fd_pin_t pin_num, fd_gpio_state_t state) {

}

/**
 * @brief Toggles the pin state.
 */
void fd_gpio_toggle(fd_pin_t pin_num) {

}

/**
 * @brief Reads the current input state of the pin.
 */
fd_gpio_state_t fd_gpio_read(fd_pin_t pin_num) {
    return FD_GPIO_LOW;
}


/**
 * @brief  Locks mutex to start an atomic section (pThread-based interrupt emulation).
 * @retval uint32_t: Lock indicator (non-zero = locked).
 * @note   Host implementation: Uses pthread_mutex_t to emulate interrupt disabling.
 */
uint32_t fd_atomic_start(void) {
    pthread_mutex_lock(&fd_atomic_mutex);
    return 1;
}

/**
 * @brief  Unlocks mutex to end an atomic section.
 * @param  primask: Lock state indicator (unused, kept for API compatibility).
 * @note   Host implementation: Uses pthread_mutex_t to emulate interrupt enabling.
 */
void fd_atomic_end(uint32_t primask) {
    (void)primask;
    pthread_mutex_unlock(&fd_atomic_mutex);
}

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
void fd_delay(uint32_t delay_ms)
{
#ifdef _WIN32
    Sleep(delay_ms);
#else
    usleep(delay_ms * 1000);
#endif
}

