#include "pico/stdlib.h"
#include <stdio.h>

const uint led_pin = 25;

int /*__attribute__ ((noreturn))*/ main()
{
    stdio_init_all();
    gpio_init(led_pin);
    gpio_set_dir(led_pin, true);
    bool state = false;;

    uint32_t i = 0;
    for (;;)
    {
        sleep_ms(1000);
        printf("Hello world! %u\n", i++);
        state = !state;
        gpio_put(led_pin, state);
    }
    __builtin_unreachable();
}