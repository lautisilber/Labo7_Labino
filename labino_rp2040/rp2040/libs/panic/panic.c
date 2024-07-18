#include "panic.h"

#ifndef NO_DEBUG
#include <stdio.h>
#endif

#include <hardware/watchdog.h>

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

static inline void sleep_long(uint8_t seconds)
{
    for(uint8_t i = 0; i < seconds; ++i)
        sleep_ms(1000);
}

inline void __attribute__ ((noreturn)) reset()
{
    watchdog_reboot(0, 0, 0);
    __builtin_unreachable();
}

void __attribute__ ((noreturn)) panic(const char *format, ...)
{
    // from https://stackoverflow.com/questions/4339412/how-to-use-va-args-inside-a-c-function-instead-of-macro
    #ifndef NO_DEBUG
    va_list args;

    va_start(args, format);
    printf(format, args);
    va_end(args);
    #endif

    sleep_long(5);
    reset();
}

void __attribute__ ((noreturn)) panic_pre_main(const char *format, ...)
{
    stdio_init_all();
    
    // from https://stackoverflow.com/questions/4339412/how-to-use-va-args-inside-a-c-function-instead-of-macro
    #ifndef NO_DEBUG
    va_list args;

    va_start(args, format);
    printf(format, args);
    va_end(args);
    #endif

    sleep_long(5);
    reset();
}

void warn(const char *format, ...)
{
    // from https://stackoverflow.com/questions/4339412/how-to-use-va-args-inside-a-c-function-instead-of-macro
    #ifndef NO_DEBUG
    va_list args;

    va_start(args, format);
    printf(format, args);
    va_end(args);
    #endif

    sleep_long(3);
}

#ifdef __cplusplus
}
#endif /* End of CPP guard */