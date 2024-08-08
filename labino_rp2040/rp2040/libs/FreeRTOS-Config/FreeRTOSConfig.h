/*
 * FreeRTOS
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://aws.amazon.com/freertos
 * https://www.freertos.org/Documentation/02-Kernel/03-Supported-devices/02-Customization#configuse_port_optimised_task_selection
 * https://github.com/FreeRTOS/FreeRTOS-Community-Supported-Demos/tree/main/CORTEX_M0%2B_RP2040
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H


/* Using FreeRTOS flag */
#define FREE_RTOS_INSTALLED

/* Use Pico SDK ISR handlers */
#define vPortSVCHandler         isr_svcall
#define xPortPendSVHandler      isr_pendsv
#define xPortSysTickHandler     isr_systick

#define configUSE_PREEMPTION                    0           // Allow tasks to be pre-empted TODO: Change this to be Cooperative (0) (https://www.freertos.org/Why-FreeRTOS/Features-and-demos/RAM_constrained_design_tutorial/solution4)
#define configUSE_TIME_SLICING                  1           // Allow FreeRTOS to switch tasks at each tick
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0           // TODO: test if 1 works, should be more efficient
#define configUSE_TICKLESS_IDLE                 0           // 1 to go to sleep every time the mcu idles... maybe not great idea for this case
#define configCPU_CLOCK_HZ                      133000000   // 133MHz for RP2040
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) // FreeRTOS beats per second -> 1000 Hz means 1 tick = 1 ms
#define configMAX_PRIORITIES                    32
#define configMINIMAL_STACK_SIZE                ( configSTACK_DEPTH_TYPE ) 256
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0           // defaults to 32 bits. Since RP2040 is 32 bits, no performance gain
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

#define configSTACK_DEPTH_TYPE                  uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t      // Defaults to size_t for backward compatibility,
                                                            // but can be changed if lengths will always be less
                                                            // than the number of bytes in a size_t.

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION         FREE_RTOS_STATIC           // I really want to use static memory allocation
#define configSUPPORT_DYNAMIC_ALLOCATION        FREE_RTOS_DYNAMIC          // TODO: I'll try to use only static memory. Get FreeRTOS to allocation task memory

#define configAPPLICATION_ALLOCATED_HEAP        0
#define configTOTAL_HEAP_SIZE                   (32*1024)   // I reduced this size to try to use static allocation

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                     0           // 1 to allow one function run every time the processor idles
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* Software timer related definitions. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            1024

// /* SMP port only */
// #define configNUM_CORES                         2
// #define configTICK_CORE                         0
// #define configRUN_MULTIPLE_PRIORITIES           0

/* RP2040 specific */
#define configSUPPORT_PICO_SYNC_INTEROP         1 // don't want to use these while working with FreeRTOS
#define configSUPPORT_PICO_TIME_INTEROP         1 // don't want to use these while working with FreeRTOS

/* Memory protection unit MPU*/
#define configENABLE_MPU                        0

#include <assert.h>
/* Define to trap errors during development. */
#define configASSERT(x)                         assert(x) // shouldn't use this on production. static_assert is ok tho

/* Optional functions - most linkers will remove unused functions anyway. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1

/* A header file that defines trace macro can be included here. */

#endif /* FREERTOS_CONFIG_H */