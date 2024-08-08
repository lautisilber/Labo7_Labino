#include "user_flash.h"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <string.h>

#include "FreeRTOS_Static.h"
#include "task.h"

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif


static bool calculate_absolute_offset(uint32_t location, size_t length, uint32_t *absolute_offset)
{
    *absolute_offset = location + USER_FLASH_SAVE_BEGIN_ADRESS;
    return *absolute_offset >= USER_FLASH_SAVE_BEGIN_ADRESS && *absolute_offset + length < PICO_FLASH_SIZE_BYTES;
}

#ifndef FREERTOS_INSTALLED
struct _UserFlashData
{
    uint32_t absolute_offset;
    uint8_t *data;
    size_t length;
};

void _user_flash_save_safe_callback(void *data)
{
    struct _UserFlashData *user_flash_data = (struct _UserFlashData *)data;
    flash_range_program(user_flash_data->absolute_offset, user_flash_data->data, user_flash_data->length);
}
#endif

bool user_flash_save(uint32_t location, const uint8_t *data, size_t length)
{
    #ifdef FREERTOS_INSTALLED

    uint32_t absolute_offset;
    if (!calculate_absolute_offset(location, length, &absolute_offset))
        return false;

    // enter critical
    taskENTER_CRITICAL();
    // critical section
    flash_range_program(absolute_offset, data, length);
    // exit critical
    taskEXIT_CRITICAL();

    return true;

    #else

    _UserFlashData user_flash_data{.absolute_offset=0, .data=data, .length=length};
    if (!calculate_absolute_offset(location, length, &user_flash_data.absolute_offset))
        return false;
    int res = flash_safe_execute(_user_flash_save_safe_callback, (void *)&user_flash_data, 1000); // timeout is 1000 ms
        #ifdef D_ERROR
        switch (res)
        {
        case PICO_OK:
            return true;
        case PICO_TIMEOUT:
            ERROR_PRINTFLN("Couldn't write to user flash in location %u due to timeout.", user_flash_data.absolute_diff);
            break;
        case PICO_ERROR_NOT_PERMITTED:
            ERROR_PRINTFLN("Couldn't write to user flash in location %u. Safe execution is not possible.", user_flash_data.absolute_diff);
            break;
        case PICO_ERROR_INSUFFICIENT_RESOURCES:
            ERROR_PRINTFLN("Couldn't write to user flash in location %u due to dynamic resource exhaustion.", user_flash_data.absolute_diff);
            break;
        default:
            break;
        }
        return false;
        #else
        return res == PICO_OK;
        #endif

    #endif

    // uint32_t absolute_offset;
    // if (!calculate_absolute_offset(&absolute_offset))
    //     return false;

    // // enter critical
    // #ifdef FREERTOS_INSTALLED
    // taskENTER_CRITICAL();
    // #else
    // uint32_t interrupts = save_and_disable_interrupts();
    // #endif
    // // critical section
    // flash_range_program(absolute_offset, data, length);
    // // exit critical
    // #ifdef FREERTOS_INSTALLED
    // taskEXIT_CRITICAL();
    // #else
    // restore_interrupts(interrupts);
    // #endif
}

bool user_flash_load(uint32_t location, uint8_t *data, size_t length)
{
    uint32_t absolute_offset;
    if (!calculate_absolute_offset(location, length, &absolute_offset))
        return false;

    // Compute the memory-mapped address, remembering to include the offset for RAM
    uint8_t *addr = (uint8_t *)XIP_BASE + absolute_offset;
    
    // enter critical
    #ifdef FREERTOS_INSTALLED
    taskENTER_CRITICAL();
    #else
    uint32_t interrupts = save_and_disable_interrupts();
    #endif
    // critical section
    memcpy(data, addr, length);
    // exit critical
    #ifdef FREERTOS_INSTALLED
    taskEXIT_CRITICAL();
    #else
    restore_interrupts(interrupts);
    #endif

    return true;
}

#ifdef __cplusplus
}
#endif /* End of CPP guard */