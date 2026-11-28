#include "Drv.h"
#include <pthread.h>
#include <stdint.h>
#include "SilServer.h"
#include "SilTick.h"

pthread_mutex_t fd_atomic_mutex;
FdDrv DRV;

void FdDrv_Init(FdDrv *h)
{
    SilTick::Init(SilTick::Mode::INTERNAL);
}

__attribute__((weak)) void FdDrv_DwtInit(){};
__attribute__((weak)) uint32_t FdDrv_DwtTick(void){return 0;};
/* Static Inline Implementation */

/**
 * @brief  Locks mutex to start an atomic section (pThread-based interrupt emulation).
 * @retval uint32_t: Lock indicator (non-zero = locked).
 * @note   Host implementation: Uses pthread_mutex_t to emulate interrupt disabling.
 */
uint32_t FdDrv_AtomicStart(void) {
    pthread_mutex_lock(&fd_atomic_mutex);
    return 1;
}

/**
 * @brief  Unlocks mutex to end an atomic section.
 * @param  primask: Lock state indicator (unused, kept for API compatibility).
 * @note   Host implementation: Uses pthread_mutex_t to emulate interrupt enabling.
 */
void FdDrv_AtomicEnd(uint32_t primask) {
    (void)primask;
    pthread_mutex_unlock(&fd_atomic_mutex);
}

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
void FdDrv_Delay(uint32_t delay_ms)
{
#ifdef _WIN32
    Sleep(delay_ms);
#else
    usleep(delay_ms * 1000);
#endif
}

