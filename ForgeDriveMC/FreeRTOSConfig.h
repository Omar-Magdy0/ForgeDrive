#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#include "System.h"

/******************************************************************************/
/* Hardware description related definitions. **********************************/
/******************************************************************************/
#define configCPU_CLOCK_HZ    FD_CPU_CLK
/*
 #define configSYSTICK_CLOCK_HZ                  [Platform specific]
 */
/******************************************************************************/
/* Scheduling behaviour related definitions. **********************************/
/******************************************************************************/
#define configTICK_RATE_HZ                         1000
#define configUSE_PREEMPTION                       1
#define configUSE_TIME_SLICING                     0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
#define configUSE_TICKLESS_IDLE                    0
#define configMAX_PRIORITIES                       5
#define configMINIMAL_STACK_SIZE                   128
#define configMAX_TASK_NAME_LEN                    16
#define configTICK_TYPE_WIDTH_IN_BITS              TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                    1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES      1
#define configQUEUE_REGISTRY_SIZE                  0
#define configENABLE_BACKWARD_COMPATIBILITY        0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS    0
#define configUSE_MINI_LIST_ITEM                   1
#define configSTACK_DEPTH_TYPE                     size_t
#define configMESSAGE_BUFFER_LENGTH_TYPE           size_t
#define configHEAP_CLEAR_MEMORY_ON_FREE            1
#define configSTATS_BUFFER_MAX_LENGTH              0xFFFF
#define configUSE_NEWLIB_REENTRANT                 0
/******************************************************************************/
/* Software timer related definitions. ****************************************/
/******************************************************************************/
#define configUSE_TIMERS                0
#define configTIMER_TASK_PRIORITY       ( configMAX_PRIORITIES - 1 )
#define configTIMER_TASK_STACK_DEPTH    configMINIMAL_STACK_SIZE
#define configTIMER_QUEUE_LENGTH        0
/******************************************************************************/
/* Event Group related definitions. *******************************************/
/******************************************************************************/
#define configUSE_EVENT_GROUPS    0
/******************************************************************************/
/* Stream Buffer related definitions. *****************************************/
/******************************************************************************/
#define configUSE_STREAM_BUFFERS    0
/******************************************************************************/
/* Memory allocation related definitions. *************************************/
/******************************************************************************/
#define configSUPPORT_STATIC_ALLOCATION              1
#define configSUPPORT_DYNAMIC_ALLOCATION             0
#define configTOTAL_HEAP_SIZE                        0
#define configAPPLICATION_ALLOCATED_HEAP             0
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP    0
#define configENABLE_HEAP_PROTECTOR                  0
/******************************************************************************/
/* Interrupt nesting behaviour configuration. *********************************/
/******************************************************************************/
#if defined(__NVIC_PRIO_BITS)
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   0xf
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY   ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY  ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
    
#define configMAX_API_CALL_INTERRUPT_PRIORITY    configMAX_SYSCALL_INTERRUPT_PRIORITY
/******************************************************************************/
/* Hook and callback function related definitions. ****************************/
/******************************************************************************/
#define configUSE_IDLE_HOOK                   0
#define configUSE_TICK_HOOK                   0
#define configUSE_MALLOC_FAILED_HOOK          0
#define configUSE_DAEMON_TASK_STARTUP_HOOK    0
#define configUSE_SB_COMPLETED_CALLBACK       0

#if defined(FD_DEBUG)
    #define configCHECK_FOR_STACK_OVERFLOW        2
    #define configGENERATE_RUN_TIME_STATS         0
    #define configUSE_TRACE_FACILITY              1
    #define configUSE_STATS_FORMATTING_FUNCTIONS  0
#else
    #define configCHECK_FOR_STACK_OVERFLOW        2
    #define configGENERATE_RUN_TIME_STATS         0
    #define configUSE_TRACE_FACILITY              0
    #define configUSE_STATS_FORMATTING_FUNCTIONS  0
#endif
/******************************************************************************/
/* Co-routine related definitions. ********************************************/
/******************************************************************************/
#define configUSE_CO_ROUTINES              0
#define configMAX_CO_ROUTINE_PRIORITIES    1
/******************************************************************************/
/* Debugging assistance. ******************************************************/
/******************************************************************************/
#define configASSERT( x )         \
    if( ( x ) == 0 )              \
    {                             \
        taskDISABLE_INTERRUPTS(); \
        for( ; ; )                \
        ;                         \
    }
/******************************************************************************/
/* FreeRTOS MPU specific definitions. *****************************************/
/******************************************************************************/
#define configINCLUDE_APPLICATION_DEFINED_PRIVILEGED_FUNCTIONS    0
#define configTOTAL_MPU_REGIONS                                   8
#define configTEX_S_C_B_FLASH                                     0x07UL
#define configTEX_S_C_B_SRAM                                      0x07UL
#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY               1
#define configALLOW_UNPRIVILEGED_CRITICAL_SECTIONS                0
#define configUSE_MPU_WRAPPERS_V1                                 0
#define configPROTECTED_KERNEL_OBJECT_POOL_SIZE                   10
#define configSYSTEM_CALL_STACK_SIZE                              128
#define configENABLE_ACCESS_CONTROL_LIST                          1
/******************************************************************************/
/* SMP( Symmetric MultiProcessing ) Specific Configuration definitions. *******/
/******************************************************************************/
/*
 #define configNUMBER_OF_CORES                     [Num of available cores]
 */
#define configRUN_MULTIPLE_PRIORITIES             0
#define configUSE_CORE_AFFINITY                   0
#define configTASK_DEFAULT_CORE_AFFINITY          tskNO_AFFINITY
#define configUSE_TASK_PREEMPTION_DISABLE         0
#define configUSE_PASSIVE_IDLE_HOOK               0
#define configTIMER_SERVICE_TASK_CORE_AFFINITY    tskNO_AFFINITY
#define configIDLE_AFFINITY                       0
/******************************************************************************/
/* ARMv8-M secure side port related definitions. ******************************/
/******************************************************************************/
#define secureconfigMAX_SECURE_CONTEXTS        5
#define configKERNEL_PROVIDED_STATIC_MEMORY    1
/******************************************************************************/
/* ARMv8-M port Specific Configuration definitions. ***************************/
/******************************************************************************/
#define configENABLE_TRUSTZONE            1
#define configRUN_FREERTOS_SECURE_ONLY    1
#define configENABLE_MPU                  1
#define configENABLE_FPU                  1
#define configENABLE_MVE                  1
/******************************************************************************/
/* ARMv7-M and ARMv8-M port Specific Configuration definitions. ***************/
/******************************************************************************/
#define configCHECK_HANDLER_INSTALLATION    0
#if defined(__CORTEX_M)
    #define vPortSVCHandler       SVC_Handler
    #define xPortPendSVHandler    PendSV_Handler
    #define xPortSysTickHandler   SysTick_Handler
#endif
/******************************************************************************/
/* Definitions that include or exclude functionality. *************************/
/******************************************************************************/
#define configUSE_TASK_NOTIFICATIONS           1
#define configUSE_MUTEXES                      1
#define configUSE_RECURSIVE_MUTEXES            0
#define configUSE_COUNTING_SEMAPHORES          0
#define configUSE_QUEUE_SETS                   0
#define configUSE_APPLICATION_TASK_TAG         0
#define configUSE_POSIX_ERRNO                  0
#define INCLUDE_vTaskPrioritySet               0
#define INCLUDE_uxTaskPriorityGet              0
#define INCLUDE_vTaskDelete                    0
#define INCLUDE_vTaskSuspend                   0
#define INCLUDE_xTaskDelayUntil                1
#define INCLUDE_vTaskDelay                     1
#define INCLUDE_xTaskGetSchedulerState         1
#define INCLUDE_xTaskGetCurrentTaskHandle      1
#define INCLUDE_uxTaskGetStackHighWaterMark    1
#define INCLUDE_xTaskGetIdleTaskHandle         0
#define INCLUDE_eTaskGetState                  0
#define INCLUDE_xTimerPendFunctionCall         0
#define INCLUDE_xTaskAbortDelay                0
#define INCLUDE_xTaskGetHandle                 0
#define INCLUDE_xTaskResumeFromISR             1

#endif /* FREERTOS_CONFIG_H */
