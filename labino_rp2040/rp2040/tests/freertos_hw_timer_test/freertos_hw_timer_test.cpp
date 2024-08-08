#include "pico/stdlib.h"
#include "debug_helper.h"
#include "pico/time.h"
#include <stdio.h>


#include "FreeRTOS_Static.h"
#include "task.h"

void print_task_freertos(void *pvParameters);
bool print_task_hw(repeating_timer_t *rt);

const size_t stack_size = FREE_RTOS_TASK_MIN_STACK_SIZE(sizeof(uint8_t) + sizeof(char)*32);

StackType_t freertos_task_stack[stack_size];
StaticTask_t freertos_task_buffer;

static uint32_t counter = 0;

int main(void)
{
    stdio_init_all();

    sleep_ms(1000);
    printf("Hello, world multitask! %u\n");

    /*
        BaseType_t xTaskCreate( TaskFunction_t pvTaskCode,
                        const char * const pcName,
                        const configSTACK_DEPTH_TYPE uxStackDepth,
                        void *pvParameters,
                        UBaseType_t uxPriority,
                        TaskHandle_t *pxCreatedTask
                    );

        TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                        const char * const pcName,
                        const uint32_t ulStackDepth,
                        void * const pvParameters,
                        UBaseType_t uxPriority,
                        StackType_t * const puxStackBuffer,
                        StaticTask_t * const pxTaskBuffer
                    );

    */

    xTaskCreateStatic(
        print_task_freertos,
        "static_1",
        stack_size,
        NULL,
        1,
        freertos_task_stack,
        &freertos_task_buffer
    );


    printf("Starting timer\n");
    repeating_timer_t timer_handler;
    add_repeating_timer_us(35, print_task_hw, &counter, &timer_handler);

    printf("Starting scheduler\n");
    vTaskStartScheduler();

    CRITICAL_PRINTLN("Shouldn't have reached this section!");
}

void print_task_freertos(void *pvParameters)
{
    uint32_t old_counter_value = 0;
    for (;;)
    {
        printf("freertos %u (diff %u)\n", counter, counter - old_counter_value);
        old_counter_value = counter;
        vTaskDelay(1000); // 1 tick = 1 ms at 1000 Hz
    }
}

bool print_task_hw(repeating_timer_t *rt)
{
    uint32_t *counter = (uint32_t *)rt->user_data;
    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    // printf("hw_timer %u\n", *counter);
    *counter += 1;
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    return true; // true = repeat
}