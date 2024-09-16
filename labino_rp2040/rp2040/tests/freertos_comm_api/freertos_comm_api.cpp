#include "pico/stdlib.h"
#include "debug_helper.h"
#include <stdio.h>
#include "comm_basic.hpp"

#include "FreeRTOS_Static.h"
#include "task.h"

static const char *msgs[] = {
    "comm1 arg1 arg2 arg3\n",
    "comm2\n",
    "comm3 carg1 carg2 carg3 carg4 carg5 carg6",
    "comm4 darg1 darg2 darg3 darg4 darg5 darg6 darg7\n",
    "comm5 sadfilbaisdufhnvlaisdhfnlvihlnfvidsauhnlasdfv\n",
    "comm6 sdfg \n",
    "chach\n",
    "kak",
    "pip ",
    " tut"
};
static const size_t n_msgs = sizeof(msgs) / sizeof(msgs[0]);

CommAPI_Basic api('\n', ' ');

void api_default_response(const char *command, const char **arguments, size_t n_arguments)
{
    api.send_message("Received message");
    
    printf("\tCommand: %s\n", command);
    for (size_t i = 0; i < n_arguments; i++)
    {
        printf("\tArgument %u: %s\n", i, arguments[i]);
    }
}

void task_new_msg(void *pvParameters);
void task_process_existing_msg(void *pvParameters);

const size_t stack_size = 1024;

StackType_t task_stack[stack_size];
StaticTask_t task_buffer;

int main()
{
    stdio_init_all();
    api.add_default_response(api_default_response);

    sleep_ms(1000);
    stdio_puts("Comm API test begin\n");

    TaskHandle_t xHandle = NULL;

    xHandle = xTaskCreateStatic(
        task_new_msg,
        "process_msg",
        stack_size,
        NULL,
        2,
        task_stack,
        &task_buffer
    );

    TaskHandle_t xProcHandle = api.create_message_processing_task(1, 15);

    printf("Starting scheduler\n");
    vTaskStartScheduler();

    CRITICAL_PRINTLN("Shouldn't have reached this section!");
}

void task_new_msg(void *pvParameters)
{
    size_t i = 0;
    for (;;)
    {
        api.save_new_message(msgs[i++]);
        if (i >= n_msgs) i = 0;
        vTaskDelay(1000);
    }
}