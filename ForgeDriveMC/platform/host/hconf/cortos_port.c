#include "cortos.h"
#include "platform.h"
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

static pthread_mutex_t cortos_critical_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t idle_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t idle_cond = PTHREAD_COND_INITIALIZER;
static bool idle_wakeup = false;

uint32_t CORTOS_PortProfTick_rate_hz(){return 1000000000;} 

extern void Ticker_IRQ();
void CORTOS_PortTickerInit()
{
//Here register a software timer associated with ISR and hand it IRQ
    register_timer(&timer_manager, Ticker_IRQ, (1000000000)/(cortosTICK_RATE_HZ));
}
void CORTOS_PortProfTickerInit(){}
void CORTOS_PortIdle()
{
    pthread_mutex_lock(&idle_mutex);
    while (!idle_wakeup)
    {
        pthread_cond_wait(&idle_cond, &idle_mutex);
    }
    idle_wakeup = false;
    pthread_mutex_unlock(&idle_mutex);
}
void CORTOS_PortWake(void)
{
    pthread_mutex_lock(&idle_mutex);
    idle_wakeup = true;
    pthread_cond_signal(&idle_cond);
    pthread_mutex_unlock(&idle_mutex);
}

uint32_t CORTOS_PortEnterCritical()
{
    pthread_mutex_lock(&cortos_critical_mutex);
    return 0;  
}

void CORTOS_PortExitCritical(uint32_t qstate)
{
    (void)qstate;
    pthread_mutex_unlock(&cortos_critical_mutex);
}

uint32_t CORTOS_PortProfTicks()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}