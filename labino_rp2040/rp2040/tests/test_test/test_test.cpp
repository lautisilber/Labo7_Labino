#include "pico/stdlib.h"
#include <stdio.h>

int __attribute__ ((noreturn)) main()
{
    stdio_init_all();

    uint32_t i = 0;
    for (;;)
    {
        sleep_ms(1000);
        printf("Hello world! %u\n", i++);
    }
    __builtin_unreachable();
}