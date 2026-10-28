#include "fd_drv.h"
#include <pthread.h>
#include <stdint.h>

pthread_mutex_t fd_atomic_mutex;
fd_drv_t DRV;

void fd_drv_init(fd_drv_t *h){}

__attribute__((weak)) void fd_dwt_init(){};
__attribute__((weak)) uint32_t fd_dwt_tick(void){return 0;};
/* Static Inline Implementation */

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

