#include "pico/stdlib.h"
#include "debug_helper.h"
#include <stdio.h>


#include "FreeRTOS_Static.h"
#include "task.h"

const uint led_pin = 25;

void led_toggle_task(void *pvParameters);

const size_t stack_size = 1024; //sizeof(TaskData) + sizeof(uint8_t) + sizeof(TaskData *);

StackType_t task_stack[stack_size];
StaticTask_t task_buffer;

int main(void)
{
    stdio_init_all();
    gpio_init(led_pin);
    gpio_set_dir(led_pin, true);

    sleep_ms(1000);
    printf("Hello, world static!\n");

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

    TaskHandle_t xHandle = NULL;

    xHandle = xTaskCreateStatic(
        led_toggle_task,
        "toggle",
        stack_size,
        NULL,
        1,
        task_stack,
        &task_buffer
    );

    printf("Starting scheduler\n");
    vTaskStartScheduler();

    CRITICAL_PRINTLN("Shouldn't have reached this section!");
}

void led_toggle_task(void *pvParameters)
{
    bool state;
    for (;;)
    {
        state = !gpio_get(led_pin);
        gpio_put(led_pin, state);
        printf("Led static %u\n", state);
        vTaskDelay(500);
    }
}