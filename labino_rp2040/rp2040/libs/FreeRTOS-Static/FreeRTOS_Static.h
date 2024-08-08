#ifndef FREE_RTOS_STATIC_H
#define FREE_RTOS_STATIC_H

#include "FreeRTOS.h"
#if configSUPPORT_STATIC_ALLOCATION
#include "task.h"
#include "timers.h"

#define FREE_RTOS_TASK_MIN_STACK_SIZE(vars_size) MAX(vars_size, configMINIMAL_STACK_SIZE)

// #define FREE_RTOS_IDLE_TASK_SIZE configMINIMAL_STACK_SIZE * 2

// /* static memory allocation for the IDLE task */
// static StaticTask_t xIdleTaskTCBBuffer;
// static StackType_t xIdleStack[FREE_RTOS_IDLE_TASK_SIZE];

// void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, configSTACK_DEPTH_TYPE *pulIdleTaskStackSize) {
//   *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
//   *ppxIdleTaskStackBuffer = &xIdleStack[0];
//   *pulIdleTaskStackSize = FREE_RTOS_IDLE_TASK_SIZE;
// }
// #endif

// #if configSUPPORT_STATIC_ALLOCATION && configUSE_TIMERS
// static StaticTask_t xTimerTaskTCBBuffer;
// static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

// /* If static allocation is supported then the application must provide the
//    following callback function - which enables the application to optionally
//    provide the memory that will be used by the timer task as the task's stack
//    and TCB. */
// void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, configSTACK_DEPTH_TYPE *pulTimerTaskStackSize) {
//   *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
//   *ppxTimerTaskStackBuffer = &xTimerStack[0];
//   *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
// }

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
   implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
   used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
       function then they must be declared static - otherwise they will be allocated on
       the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
       state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
       Note that, as the array is necessarily of type StackType_t,
       configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/*-----------------------------------------------------------*/

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
   application must provide an implementation of vApplicationGetTimerTaskMemory()
   to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     configSTACK_DEPTH_TYPE *pulTimerTaskStackSize )
{
    /* If the buffers to be provided to the Timer task are declared inside this
       function then they must be declared static - otherwise they will be allocated on
       the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
       task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
       Note that, as the array is necessarily of type StackType_t,
      configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif
#endif /* FREE_RTOS_STATIC_H */