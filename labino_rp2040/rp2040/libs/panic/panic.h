#ifndef PANIC_H
#define PANIC_H

// if you define NO_DEBUG, no debug messages will be written to stdout

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

// #define NO_DEBUG

#include <pico/stdlib.h>

#ifndef NO_DEBUG
#include <stdarg.h>
#endif

inline void  __attribute__ ((noreturn)) reset();
void __attribute__ ((noreturn)) panic(const char *format, ...);
void warn(const char *format, ...);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* PANIC_H */