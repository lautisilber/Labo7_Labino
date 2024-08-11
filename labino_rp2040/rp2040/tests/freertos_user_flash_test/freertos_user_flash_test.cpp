#include "pico/stdlib.h"
#include "debug_helper.h"
#include <stdio.h>
#include "user_flash_class.hpp"
#include <stdlib.h>

#include "FreeRTOS_Static.h"
#include "task.h"

#define DATA_SIZE 7

const size_t stack_size = 1024 + DATA_SIZE; //sizeof(TaskData) + sizeof(uint8_t) + sizeof(TaskData *);

StackType_t task_stack[stack_size];
StaticTask_t task_buffer;

class UserFlashClass : public UserFlashBase
{
public:
    uint8_t _data[DATA_SIZE] = {0};
    UserFlashClass() : UserFlashBase(sizeof(_data)) {}

    void write_data_ram(const uint8_t *new_data, size_t length)
    {
        memcpy(_data, new_data, MIN(length, count_of(_data)));
    }

    void save()
    {
        base_flash_save(&_data);
    }

    void load()
    {
        base_flash_load(&_data);
    }

    void clear()
    {
        for (size_t i = 0; i < count_of(_data); i++)
        {
            _data[i] = 0;
        }
    }

    void print()
    {
        for (uint i = 0; i < count_of(_data); i++)
        {
            printf("%u", _data[i]);
            if (i > count_of(_data) - 2)
            {
                printf("\n");
            }
            else
            {
                printf(", ");
            }
        }
    }
};

UserFlashClass ufc;

void task_user_flash(void *pvParameters)
{
    uint8_t new_data[DATA_SIZE];
    for (size_t i = 0; i < DATA_SIZE; i++)
        new_data[i] = rand() >> 16;

    ufc.load();
    printf("loaded\n");
    printf("preexisting data\n");
    ufc.print();
    ufc.write_data_ram(new_data, count_of(new_data));
    printf("written random data\n");
    ufc.print();
    ufc.save();
    printf("saved\n");
    ufc.clear();
    printf("cleared\n");
    ufc.load();
    printf("loaded\n");
    ufc.print();
    printf("end\n");

    for (;;)
    {
        vTaskDelay(1000);
    }
}

int main(void)
{
    stdio_init_all();

    sleep_ms(1000);

    /*
        TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                        const char * const pcName,
                        const uint32_t ulStackDepth,
                        void * const pvParameters,
                        UBaseType_t uxPriority,
                        StackType_t * const puxStackBuffer,
                        StaticTask_t * const pxTaskBuffer
                    );

    */

    TaskHandle_t xHandle = xTaskCreateStatic(
        task_user_flash,
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
