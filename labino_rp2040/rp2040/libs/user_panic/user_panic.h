#ifndef PANIC_H
#define PANIC_H

// if you define NO_DEBUG, no debug messages will be written to stdout

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

// #define NO_DEBUG

#include <pico/stdlib.h>
#include <hardware/watchdog.h>
#include "debug_helper.h"

#define USER_RESET()                        \
do                                          \
{                                           \
    watchdog_reboot(0, 0, 0);               \
    __builtin_unreachable();                \
} while (0)

#define USER_PANIC(format, ...)             \
do                                          \
{                                           \
    CRITICAL_PRINTFLN(format, __VA_ARGS__); \
    sleep_ms(5 * 1000);                     \
    USER_RESET();                           \
} while (0)

#define USER_PANIC_PRE_MAIN(format, ...)    \
do                                          \
{                                           \
    stdio_init_all();                       \
    sleep_ms(1000);                         \
    USER_PANIC(format, __VA_ARGS__);        \
} while (0)

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* PANIC_H */