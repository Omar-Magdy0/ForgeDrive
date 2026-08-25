#pragma once

#include <stdint.h>
#include <pthread.h>



#ifdef __cplusplus
extern "C"{
#endif

extern pthread_mutex_t fd_atomic_mutex;

typedef struct{



}fd_core_t;

void fd_core_init(fd_core_t *h);


static inline uint32_t fd_prof_tick()
{
    return 0;
};

static inline uint32_t fd_prof_tock(uint32_t start)
{
    (void)start;
    return 0;
};

/* Types */
/** @brief Type definition for global pin indexing. */
typedef uint16_t fd_pin_t;

/** @brief GPIO Mode enumeration mapped to LL Driver. */
typedef enum {
    FD_GPIO_MODE_INPUT,
    FD_GPIO_MODE_OUTPUT,
    FD_GPIO_MODE_ALT,
    FD_GPIO_MODE_ANALOG
} fd_gpio_mode_t;

/** @brief Logic state enumeration for GPIO pins. */
typedef enum {
    FD_GPIO_LOW  = 0,
    FD_GPIO_HIGH = 1
} fd_gpio_state_t;

/* Static Inline Implementation */

/**
 * @brief Sets the pin mode for a specific global pin number.
 */
static inline void fd_gpio_mode(fd_pin_t pin_num, fd_gpio_mode_t mode) {

}

/**
 * @brief Writes to a pin using the BSRR register (Atomic).
 * If state is HIGH, it writes to the lower 16 bits; if LOW, the upper 16 bits.
 */
static inline void fd_gpio_write(fd_pin_t pin_num, fd_gpio_state_t state) {

}

/**
 * @brief Toggles the pin state.
 */
static inline void fd_gpio_toggle(fd_pin_t pin_num) {

}

/**
 * @brief Reads the current input state of the pin.
 */
static inline fd_gpio_state_t fd_gpio_read(fd_pin_t pin_num) {
    return FD_GPIO_LOW;
}


/**
 * @brief  Locks mutex to start an atomic section (pThread-based interrupt emulation).
 * @retval uint32_t: Lock indicator (non-zero = locked).
 * @note   Host implementation: Uses pthread_mutex_t to emulate interrupt disabling.
 */
static inline uint32_t fd_atomic_start(void) {
    pthread_mutex_lock(&fd_atomic_mutex);
    return 1;
}

/**
 * @brief  Unlocks mutex to end an atomic section.
 * @param  primask: Lock state indicator (unused, kept for API compatibility).
 * @note   Host implementation: Uses pthread_mutex_t to emulate interrupt enabling.
 */
static inline void fd_atomic_end(uint32_t primask) {
    (void)primask;
    pthread_mutex_unlock(&fd_atomic_mutex);
}

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
static inline void fd_delay(uint32_t delay_ms)
{
#ifdef _WIN32
    Sleep(delay_ms);
#else
    usleep(delay_ms * 1000);
#endif
}

#ifdef __cplusplus
}
#endif
