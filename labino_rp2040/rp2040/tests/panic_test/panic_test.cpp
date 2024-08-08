#include "pico/stdlib.h"
#include "user_panic.h"

#include <stdio.h>

int main()
{
    #ifdef D_CRITICAL
    USER_PANIC_PRE_MAIN("Testing user %s!", "panic");
    #else
    stdio_init_all();
    sleep_ms(1000);
    printf("Testing user reset!\n");
    USER_RESET();
    #endif

    // stdio_init_all();
    // sleep_ms(1000);
    // printf("Testing user panic!\n");
    // USER_RESET();
}