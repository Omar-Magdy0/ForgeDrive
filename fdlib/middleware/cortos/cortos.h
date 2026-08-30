#ifndef CORTOS 
#define CORTOS

#include <stdint.h>
#include <stdlib.h>
#include "cortos_conf.h"
// TODO : Initialize a fully fledged cooperative Rtos

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CORTOS_READY,
    CORTOS_SLEEPING,
    CORTOS_WAITING_EVENT,
    CORTOS_SUSPENDED
} task_state_t;

typedef enum
{
    CORTOS_INF_TIMEOUT = 1<<0,
} cortos_task_flags;

#define CORTOS_MAX_DELAY UINT32_MAX 

typedef struct {
    void (*exec)(void *arg);
    void *arg;

    //Scheduler mechanisms
    volatile uint32_t wake_time;
    task_state_t state;
    volatile uint32_t notify_value;
    uint8_t flags;

    //Cpu time and profiling
    uint32_t exec_ptick_budget; 
    uint32_t exec_ptick_max;
    uint32_t exec_ptick_total;
    uint32_t exec_overruns;
    uint16_t exec_calls;

    uint16_t prof_calls;
    uint16_t prof_exec_q16;
} Task;

typedef struct 
{
    uint32_t ptick_last;
    uint32_t exec_ptick_max;
    uint32_t exec_ptick_total;
    uint16_t exec_calls;

    uint16_t prof_calls;
    uint16_t prof_exec_q16;
} ISRProfiler;

typedef struct {
    Task tasks[CORTOS_MAX_TASKS];
    ISRProfiler isrprof[CORTOS_MAX_ISRPROF];
    uint8_t task_count;
    
    uint32_t pticks_last;
    uint32_t prof_period_us;
    uint32_t prof_exec_q16;

    uint32_t ticks;
    Task *current;
} Scheduler;



#define xTICK_PERIOD_MS ( 1000 / cortosTICK_RATE_HZ )
#define xMS_TO_TICKS(x)((x) * xTICK_PERIOD_MS)
#define xTICKS_TO_MS(x)   ((x) * xTICK_PERIOD_MS)

#define xPROFTICK_PERIOD_NS ( 1000000000 / CORTOS_PortProfTick_rate_hz() )
#define xPROFTICKS_TO_NS(x)(((uint64_t)x * 1000000000)/CORTOS_PortProfTick_rate_hz())
#define xPROFTICKS_TO_US(x)(((uint64_t)x * 1000000)/CORTOS_PortProfTick_rate_hz())

void xISRProfilerEnter(uint8_t isr_id);
void xISRProfilerExit(uint8_t isr_id);
void xSchedulerStart();
void xSchedulerInit();
Task* xTaskCreate(void (*exec)(void *), void *arg, uint32_t exec_ptick_budget);
void xTaskNotifyWait(uint32_t timeout_ticks);
void xTaskNotify(Task *task, uint32_t value);
uint32_t xTaskNotifyTake();
void xTaskSleep(uint32_t sleep_ticks);
void xTaskResume(Task *task);
void xTaskSuspend();
uint32_t xTaskGetTickCount();
void xTaskProfiler();

#ifdef __cplusplus
}
#endif

#endif

