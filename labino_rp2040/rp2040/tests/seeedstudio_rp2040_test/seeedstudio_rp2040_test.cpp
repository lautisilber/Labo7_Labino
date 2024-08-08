#include "pico/stdlib.h"
#include <stdio.h>


const uint led_pin = 25;


int main()
{
    stdio_init_all();
    gpio_init(led_pin);
    gpio_set_dir(led_pin, true);

    bool state = false;

    for (;;)
    {
        bool state = !gpio_get(led_pin);
        gpio_put(led_pin, state);
        printf("tick %u\n", state);
        sleep_ms(500);
    }
}