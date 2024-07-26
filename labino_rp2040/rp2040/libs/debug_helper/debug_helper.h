#ifndef DEBUG_H
#define DEBUG_H

#include <pico/stdlib.h>
#include <stdio.h>

/*
    Debugging flags:
        - D_DEBUG
        - D_INFO
        - D_WARN
        - D_ERROR
        - D_CRITICAL
*/

#define D_DEBUG
#define D_INFO
#define D_WARN
#define D_ERROR
#define D_CRITICAL

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

#if (defined(D_DEBUG) || defined(D_INFO) || defined(D_WARN) || defined(D_ERROR) || defined(D_CRITICAL))
#define _D_PRINTF_HELPER(prefix, ...) do { printf("%s | %s - %s: ", __FILE__, __LINE__, prefix); printf(__VA_ARGS__); } while(0)
#endif

#ifdef D_DEBUG
#define DEBUG_PRINTF(...) _D_PRINTF_HELPER("DEBUG", __VA_ARGS__)
#define DEBUG_PRINTFLN(...) do { DEBUG_PRINTF( __VA_ARGS__); puts("\n"); } while(0)
#else
#define DEBUG_PRINTF(...)
#define DEBUG_PRINTFLN(...)
#endif
#define DEBUG_PRINT(msg) DEBUG_PRINTF(msg)
#define DEBUG_PRINTLN(msg) DEBUG_PRINTFLN(msg)

#ifdef D_INFO
#define INFO_PRINTF(...) _D_PRINTF_HELPER("INFO", __VA_ARGS__)
#define INFO_PRINTFLN(...) do { INFO_PRINTF( __VA_ARGS__); puts("\n"); } while(0)
#else
#define INFO_PRINTF(...)
#define INFO_PRINTFLN(...)
#endif
#define INFO_PRINT(msg) INFO_PRINTF(msg)
#define INFO_PRINTLN(msg) INFO_PRINTFLN(msg)

#ifdef D_WARN
#define WARN_PRINTF(...) _D_PRINTF_HELPER("WARN", __VA_ARGS__)
#define WARN_PRINTFLN(...) do { WARN_PRINTF( __VA_ARGS__); puts("\n"); } while(0)
#else
#define WARN_PRINTF(...)
#define WARN_PRINTFLN(...)
#endif
#define WARN_PRINT(msg) WARN_PRINTF(msg)
#define WARN_PRINTLN(msg) WARN_PRINTFLN(msg)

#ifdef D_ERROR
#define ERROR_PRINTF(...) _D_PRINTF_HELPER("ERROR", __VA_ARGS__)
#define ERROR_PRINTFLN(...) do { ERROR_PRINTF( __VA_ARGS__); puts("\n"); } while(0)
#else
#define ERROR_PRINTF(...)
#define ERROR_PRINTFLN(...)
#endif
#define ERROR_PRINT(msg) ERROR_PRINTF(msg)
#define ERROR_PRINTLN(msg) ERROR_PRINTFLN(msg)

#ifdef D_CRITICAL
#define CRITICAL_PRINTF(...) _D_PRINTF_HELPER("CRITICAL", __VA_ARGS__)
#define CRITICAL_PRINTFLN(...) do { CRITICAL_PRINTF( __VA_ARGS__); puts("\n"); } while(0)
#else
#define CRITICAL_PRINTF(...)
#define CRITICAL_PRINTFLN(...)
#endif
#define CRITICAL_PRINT(msg) CRITICAL_PRINTF(msg)
#define CRITICAL_PRINTLN(msg) CRITICAL_PRINTFLN(msg)


#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* DEBUG_H */