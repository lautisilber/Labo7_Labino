#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/sync.h"

#define ALARM_TIMEOUT_MS 1000

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    uint32_t *i = ((uint32_t *)user_data);
    uint32_t interrupts = save_and_disable_interrupts();
    restore_interrupts(interrupts);
    printf("Hello, alarm! %u\n", (*i)++);

    return ALARM_TIMEOUT_MS * 1000; // has to be in us
}


uint32_t i = 0;

int main()
{
    stdio_init_all();

    alarm_id_t id = add_alarm_in_ms(ALARM_TIMEOUT_MS, alarm_callback, &i, true);

    for (;;)
    {
        sleep_ms(ALARM_TIMEOUT_MS + (ALARM_TIMEOUT_MS/2));
    }
    __builtin_unreachable();
}