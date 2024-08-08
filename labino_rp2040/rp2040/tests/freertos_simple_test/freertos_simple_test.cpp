#include "pico/stdlib.h"
#include "debug_helper.h"
#include <stdio.h>

#if FREERTOS_STATIC == 1
#include "FreeRTOS_Static.h"
#else
#include "FreeRTOS.h"
#endif
#include "task.h"

const uint led_pin = 25;

void led_toggle_task(void *pvParameters);

const size_t stack_size = 1024; //sizeof(TaskData) + sizeof(uint8_t) + sizeof(TaskData *);

#if FREERTOS_STATIC == 1
StackType_t task_stack[stack_size];
#endif

int main(void)
{
    stdio_init_all();
    gpio_init(led_pin);
    gpio_set_dir(led_pin, true);

    sleep_ms(1000);
    printf("Hello world! %u\n");

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

    #if FREERTOS_STATIC == 1
    xTaskCreateStatic(
        led_toggle_task,
        "toggle",
        stack_size,
        NULL,
        1,
        task_stack,
        NULL
    );
    #else
    xTaskCreate(
        led_toggle_task,
        "toggle",
        stack_size,
        NULL,
        1,
        NULL
    );
    #endif

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
        #if FREERTOS_STATIC == 1
        printf("Led static %u\n", state);
        #else
        printf("Led dynamic %u\n", state);
        #endif
        vTaskDelay(500);
    }
}