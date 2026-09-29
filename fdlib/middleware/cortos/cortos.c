#include "cortos.h"

Scheduler cortos_scheduler;
uint32_t CORTOS_PortProfTick_rate_hz();
void CORTOS_PortTickerInit();
void CORTOS_PortProfTickerInit();
void CORTOS_PortIdle();
void CORTOS_PortWake();
uint32_t CORTOS_PortEnterCritical();
void CORTOS_PortExitCritical(uint32_t qstate);
uint32_t CORTOS_PortProfTicks();

void scheduler_execute()
{
    // Find Highest priority Ready task (priority is determined by order)
    Task *selected = NULL;
    for (uint8_t i = 0; i < cortos_scheduler.task_count; i++)
    {
        Task *task = &cortos_scheduler.tasks[i];
        if (task->state == CORTOS_READY)
        {
            selected = task;
            break;
        }
    }
    if (selected == NULL)
    {
        cortos_scheduler.current = NULL;
        CORTOS_PortIdle();
        return;
    }
    cortos_scheduler.current = selected;
    cortos_scheduler.current->exec_calls++;
    uint32_t start = CORTOS_PortProfTicks();
    selected->exec(selected->arg);
    uint32_t elapsed = CORTOS_PortProfTicks() - start;
    // post call profiling updates
    if (elapsed > cortos_scheduler.current->exec_ptick_max)
    {
        cortos_scheduler.current->exec_ptick_max = elapsed;
    }
    if (elapsed > cortos_scheduler.current->exec_ptick_budget)
    {
        cortos_scheduler.current->exec_overruns++;
    }
    cortos_scheduler.current->exec_ptick_total += elapsed;
}

void Ticker_IRQ()
{
    cortos_scheduler.ticks++;
    for (uint8_t i = 0; i < cortos_scheduler.task_count; i++)
    {
        Task *task = &cortos_scheduler.tasks[i]; // inspect tasks to be woken (timeouts/sleep)
        if (((task->state == CORTOS_SLEEPING) || (task->state == CORTOS_WAITING_EVENT)) && !(task->flags & CORTOS_INF_TIMEOUT))
        {
            if ((int32_t)(cortos_scheduler.ticks - task->wake_time) >= 0)
            {
                task->state = CORTOS_READY;
            }
        }
    }
    CORTOS_PortWake();
}

// API ============================================

void xISRProfilerEnter(uint8_t isr_id)
{
    cortos_scheduler.isrprof[isr_id].ptick_last = CORTOS_PortProfTicks();
}

void xISRProfilerExit(uint8_t isr_id)
{
    uint32_t elapsed = CORTOS_PortProfTicks() - cortos_scheduler.isrprof[isr_id].ptick_last;
    if(elapsed > cortos_scheduler.isrprof[isr_id].exec_ptick_max)cortos_scheduler.isrprof[isr_id].exec_ptick_max = elapsed;
    cortos_scheduler.isrprof[isr_id].exec_ptick_total += elapsed; 
    cortos_scheduler.isrprof[isr_id].exec_calls++;
}

void xSchedulerStart()
{
    // Initialize Scheduler and Systicker
    cortos_scheduler.ticks = 0;
    // Start Systicker and configure internals
    CORTOS_PortProfTickerInit();
    CORTOS_PortTickerInit();
    cortos_scheduler.pticks_last = CORTOS_PortProfTicks();
    while (1)
    {
        scheduler_execute();
    }
}

void xSchedulerInit()
{
    cortos_scheduler.task_count = 0;
    for(int i = 0; i < CORTOS_MAX_ISRPROF; i++)
    {
        cortos_scheduler.isrprof[i].exec_calls = 0;
        cortos_scheduler.isrprof[i].exec_ptick_max = 0;
        cortos_scheduler.isrprof[i].exec_ptick_total = 0;
    }
}

Task* xTaskCreate(void (*exec)(void *), void *arg, uint32_t exec_ptick_budget)
{
    if (cortos_scheduler.task_count < CORTOS_MAX_TASKS)
    {
        Task *task = &cortos_scheduler.tasks[cortos_scheduler.task_count];
        task->exec = exec;
        task->arg = arg;
        task->state = CORTOS_READY;
        task->exec_ptick_budget = exec_ptick_budget;
        task->flags = 0;
        task->wake_time = 0;
        task->notify_value = 0;
        task->exec_ptick_max = 0;
        task->exec_ptick_total = 0;
        task->exec_overruns = 0;
        task->exec_calls = 0;
        cortos_scheduler.task_count++;
        return task;
    }
    else
    {
        return NULL;
    }
}

void xTaskNotifyWait(uint32_t timeout_ticks)
{
    Task *task = cortos_scheduler.current;
    uint32_t qstate = CORTOS_PortEnterCritical();
    task->state = CORTOS_WAITING_EVENT;
    task->notify_value = 0;
    task->wake_time = cortos_scheduler.ticks + timeout_ticks;
    task->flags &= ~CORTOS_INF_TIMEOUT;
    if (timeout_ticks == CORTOS_MAX_DELAY)
    {
        task->flags |= CORTOS_INF_TIMEOUT;
    }
    CORTOS_PortExitCritical(qstate);
}

void xTaskNotify(Task *task, uint32_t value)
{
    uint32_t qstate = CORTOS_PortEnterCritical();
    task->notify_value |= value;
    if (task->state == CORTOS_WAITING_EVENT)
        task->state = CORTOS_READY;
    CORTOS_PortExitCritical(qstate);
    CORTOS_PortWake();
}

uint32_t xTaskNotifyTake()
{
    Task *task = cortos_scheduler.current;
    uint32_t qstate = CORTOS_PortEnterCritical();
    uint32_t value = task->notify_value;
    task->notify_value = 0;
    CORTOS_PortExitCritical(qstate);
    return value;
}



void xTaskSleep(uint32_t sleep_ticks)
{
    Task *task = cortos_scheduler.current;
    uint32_t qstate = CORTOS_PortEnterCritical();
    task->state = CORTOS_SLEEPING;
    task->wake_time = cortos_scheduler.ticks + sleep_ticks;
    task->flags &= ~CORTOS_INF_TIMEOUT;
    if (sleep_ticks == CORTOS_MAX_DELAY)
    {
        task->flags |= CORTOS_INF_TIMEOUT;
    }
    CORTOS_PortExitCritical(qstate);
}

void xTaskResume(Task *task)
{
    uint32_t qstate = CORTOS_PortEnterCritical();
    if (task->state == CORTOS_SUSPENDED)
        task->state = CORTOS_READY;
    CORTOS_PortExitCritical(qstate);
}

void xTaskSuspend()
{
    uint32_t qstate = CORTOS_PortEnterCritical();
    cortos_scheduler.current->state = CORTOS_SUSPENDED;
    CORTOS_PortExitCritical(qstate);
}

uint32_t xTaskGetTickCount()
{
    return cortos_scheduler.ticks;
}

void xTaskProfiler(void)
{
    uint32_t pticks = CORTOS_PortProfTicks();
    uint32_t prof_pticks = pticks - cortos_scheduler.pticks_last; //get profiling period
    if(prof_pticks == 0)return;
    //tasks profiling
    for(uint8_t i = 0; i < cortos_scheduler.task_count; i++)
    {
        cortos_scheduler.tasks[i].prof_calls = cortos_scheduler.tasks[i].exec_calls;
        cortos_scheduler.tasks[i].exec_calls = 0;
        cortos_scheduler.tasks[i].prof_exec_q16 = ((uint64_t)cortos_scheduler.tasks[i].exec_ptick_total * UINT16_MAX)/(prof_pticks);
        cortos_scheduler.tasks[i].exec_ptick_total = 0;
    }
    //Isr profiling
    for(uint8_t i = 0; i < CORTOS_MAX_ISRPROF; i++)
    {
        cortos_scheduler.isrprof[i].prof_calls = cortos_scheduler.isrprof[i].exec_calls;
        cortos_scheduler.isrprof[i].exec_calls = 0;
        cortos_scheduler.isrprof[i].prof_exec_q16 = ((uint64_t)cortos_scheduler.isrprof[i].exec_ptick_total * UINT16_MAX)/(prof_pticks);
        cortos_scheduler.isrprof[i].exec_ptick_total = 0;
    }

    cortos_scheduler.prof_period_us = xPROFTICKS_TO_US(prof_pticks);
    cortos_scheduler.prof_exec_q16 = 0;
    for(uint8_t i = 0; i < cortos_scheduler.task_count; i++)
    {
        cortos_scheduler.prof_exec_q16 += cortos_scheduler.tasks[i].prof_exec_q16;
    }
    cortos_scheduler.pticks_last = pticks;
}