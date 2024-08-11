#include "user_flash_no_os.h"
#include "debug_helper.h"
// #include <pico/flash.h>
#include <hardware/sync.h>
#include <string.h>

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

static bool _calculate_absolute_offset(uint32_t location, size_t length, uint32_t *absolute_offset)
{
    *absolute_offset = location + USER_FLASH_ADDRESS_BEGIN;
    return *absolute_offset >= USER_FLASH_ADDRESS_BEGIN && *absolute_offset + length < PICO_FLASH_SIZE_BYTES;
}

/*
 * struct _UserFlashData
 * {
 *     uint8_t *absolute_offset;
 *     const uint8_t *data;
 *     const size_t length;
 * };
 * 
 * For some reason flash_safe_execute not working properly. It panics every time
 * Just make sure that only one core only ever accesses XIP flash memory
 * void _user_flash_save_safe_callback(void *data)
 * {
 *     printf("B\n");
 *     struct _UserFlashData *user_flash_data = (struct _UserFlashData *)data;
 *     flash_range_program(user_flash_data->absolute_offset, user_flash_data->data, user_flash_data->length);
 *     printf("C\n");
 * }
 */

bool user_flash_save_no_os(uint32_t location, const uint8_t *data, size_t length)
{
    // struct _UserFlashData user_flash_data = {.absolute_offset=0, .data=data, .length=length};
    // if (!_calculate_absolute_offset(location, length, &user_flash_data.absolute_offset))
    //     return false;
    // int res = flash_safe_execute(_user_flash_save_safe_callback, (void *)&user_flash_data, 1000); // timeout is 1000 ms
    // int res = PICO_OK;
    // #ifdef D_ERROR
    // switch (res)
    // {
    // case PICO_OK:
    //     return true;
    // case PICO_TIMEOUT:
    //     ERROR_PRINTFLN("Couldn't write to user flash in location %u due to timeout.", user_flash_data.absolute_diff);
    //     break;
    // case PICO_ERROR_NOT_PERMITTED:
    //     ERROR_PRINTFLN("Couldn't write to user flash in location %u. Safe execution is not possible.", user_flash_data.absolute_diff);
    //     break;
    // case PICO_ERROR_INSUFFICIENT_RESOURCES:
    //     ERROR_PRINTFLN("Couldn't write to user flash in location %u due to dynamic resource exhaustion.", user_flash_data.absolute_diff);
    //     break;
    // default:
    //     break;
    // }
    // return false;
    // #else
    // return res == PICO_OK;
    // #endif

    if (length % FLASH_PAGE_SIZE != 0)
    {
        ERROR_PRINTFLN("Couldn't write to user flash: length (%u) isn't multiple of FLASH_PAGE_SIZE (%u)", length, FLASH_PAGE_SIZE);
        return false;
    }
    uint32_t absolute_offset;
    if (!_calculate_absolute_offset(location, length, &absolute_offset))
    {
        ERROR_PRINTFLN("Couldn't write to user flash: defined location [%u, %u) isn't in the region [%u, %u).", location, location + length, USER_FLASH_ADDRESS_BEGIN, PICO_FLASH_SIZE_BYTES);
        return false;
    }

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_program(absolute_offset, data, length);
    restore_interrupts(interrupts);

    return true;
}

bool user_flash_load_no_os(uint32_t location, uint8_t *data, size_t length)
{
    uint32_t absolute_offset;
    if (!_calculate_absolute_offset(location, length, &absolute_offset))
    {
        ERROR_PRINTFLN("Couldn't load from user flash: defined location [%u, %u) isn't in the region [%u, %u).", location, location + length, USER_FLASH_ADDRESS_BEGIN, PICO_FLASH_SIZE_BYTES);
        return false;
    }

    // Compute the memory-mapped address, remembering to include the offset for RAM
    const uint8_t *addr = (const uint8_t *)(XIP_BASE + absolute_offset);

    // enter critical
    uint32_t interrupts = save_and_disable_interrupts();
    // critical section
    memcpy(data, addr, length);
    // exit critical
    restore_interrupts(interrupts);

    return true;
}

bool user_flash_erase(uint32_t location, size_t length)
{
    if (length % FLASH_SECTOR_SIZE != 0)
    {
        ERROR_PRINTFLN("Couldn't erase user flash: length (%u) isn't multiple of FLASH_SECTOR_SIZE (%u)", length, FLASH_PAGE_SIZE);
        return false;
    }
    uint32_t absolute_offset;
    if (!_calculate_absolute_offset(location, length, &absolute_offset))
    {
        ERROR_PRINTFLN("Couldn't erase user flash: defined location [%u, %u) isn't in the region [%u, %u).", location, location + length, USER_FLASH_ADDRESS_BEGIN, PICO_FLASH_SIZE_BYTES);
        return false;
    }

    // enter critical
    uint32_t interrupts = save_and_disable_interrupts();
    // critical section
    flash_range_erase(absolute_offset, length);
    // exit critical
    restore_interrupts(interrupts);

    return true;
}

#ifdef __cplusplus
}
#endif /* End of CPP guard */