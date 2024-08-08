#include "pico/stdlib.h"
#include "debug_helper.h"
#include <stdio.h>


#include "FreeRTOS_Static.h"
#include "task.h"

void print_task(void *pvParameters);

const char *msgs[3] = { "1_static", "2_static", "3_dynamic" };

struct TaskData
{
    TickType_t tick_delay;
    const char *msg;
};

const size_t stack_size = MAX((sizeof(TaskData) + sizeof(uint8_t) + sizeof(TaskData *)), configMINIMAL_STACK_SIZE);

StackType_t stack_task_1[stack_size];
StackType_t stack_task_2[stack_size];
StaticTask_t task_1_buffer;
StaticTask_t task_2_buffer;

struct TaskData task_1_data = { .tick_delay=500, .msg=msgs[0] };
struct TaskData task_2_data = { .tick_delay=733, .msg=msgs[1] };
struct TaskData task_3_data = { .tick_delay=987, .msg=msgs[2] };

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
        print_task,
        "static_1",
        stack_size,
        &task_1_data,
        1,
        stack_task_1,
        &task_1_buffer
    );

    xTaskCreateStatic(
        print_task,
        "static_2",
        stack_size,
        &task_2_data,
        1,
        stack_task_2,
        &task_2_buffer
    );

    xTaskCreate(
        print_task,
        "dynam_3",
        stack_size,
        &task_3_data,
        1,
        NULL
    );

    printf("Starting scheduler\n");
    vTaskStartScheduler();

    CRITICAL_PRINTLN("Shouldn't have reached this section!");
}

void print_task(void *pvParameters)
{
    uint8_t counter = 0;
    struct TaskData *task_data = (struct TaskData *)pvParameters;
    for (;;)
    {
        printf("%s (%u)\n", task_data->msg, counter++);
        vTaskDelay(task_data->tick_delay);
    }
}