#include "user_flash_class.hpp"
#include "debug_helper.h"
#include <hardware/sync.h>
#include <hardware/flash.h>
#include <string.h>
#include "user_panic.h"

#if __has_include("FreeRTOS_Static.h") || __has_include("FreeRTOS.h")
#define _USER_FLASH_FREE_RTOS
#endif

#ifdef _USER_FLASH_FREE_RTOS
// Pay attention that FreeRTOS_Static should NOT be called inside libraries,
// only from main program to avoid multiple definitions of the required functions
// for FreeRTOS static allocation
// #include "FreeRTOS_Static.h"
#include "FreeRTOS.h"
#include "task.h"
#endif

static_assert(FLASH_PAGE_SIZE < FLASH_SECTOR_SIZE);
static_assert(FLASH_SECTOR_SIZE % FLASH_PAGE_SIZE == 0);

// It could be the case that these check may not be made and USER_FLASH_SIZE can be used instead of
// FLASH_PAGE_SIZE and/or FLASH_SECTOR_SIZE for calculations.
static_assert(USER_FLASH_SIZE >= FLASH_PAGE_SIZE);
static_assert(USER_FLASH_SIZE >= FLASH_SECTOR_SIZE);

static uint8_t erase_buffer[USER_FLASH_SIZE];

static constexpr uint32_t _smallest_multiple_greater_or_equal_to_x(uint32_t x, uint32_t y) {
    // Calculate remainder when x is divided by y
    uint32_t remainder = x % y;

    // If x is already a multiple of y, return x itself
    if (remainder == 0) {
        return x;
    }
    
    // Otherwise, add the difference to reach the next multiple of y
    return x + (y - remainder);
}

static bool _calculate_absolute_offset(uint32_t location, size_t length, uint32_t *absolute_offset)
{
    *absolute_offset = location + USER_FLASH_LOCATION_BEGIN;
    return *absolute_offset >= USER_FLASH_LOCATION_BEGIN && *absolute_offset + length < PICO_FLASH_SIZE_BYTES;
}


UserFlashBase::UserFlashBase(uint32_t flash_user_size)
    : _flash_user_size(flash_user_size), _flash_offset(UserFlashBase::_moving_user_flash_end + USER_FLASH_LOCATION_BEGIN),
      _flash_offset_addr(UserFlashBase::_moving_user_flash_end + USER_FLASH_LOCATION_BEGIN + (uint8_t *)XIP_BASE),
      _flash_user_page_span(_smallest_multiple_greater_or_equal_to_x(flash_user_size, MIN(FLASH_PAGE_SIZE, FLASH_PAGE_SIZE))),
      _flash_user_erase_span(_smallest_multiple_greater_or_equal_to_x(_flash_user_page_span, MIN(FLASH_SECTOR_SIZE, USER_FLASH_SIZE)))
{
    if (UserFlashBase::_moving_user_flash_end + _flash_user_erase_span > USER_FLASH_SIZE) // _flash_user_sector_span >= _flash_user_page_span always
    {
        USER_PANIC_PRE_MAIN("Couldn't assign user flash of size %u at starting location of %u due to overflow (total assigned user flash size %u; page size %u; sector size %u; begin user flash addr %u; end flash addr %u)", _flash_user_size, _flash_offset, USER_FLASH_SIZE, FLASH_PAGE_SIZE, FLASH_SECTOR_SIZE, USER_FLASH_LOCATION_BEGIN, PICO_FLASH_SIZE_BYTES);
    }
    UserFlashBase::_moving_user_flash_end += _flash_user_page_span;
}


void UserFlashBase::base_flash_save(const void* data)
{
    // save sector for erasing
    // enter critical
    #ifdef _USER_FLASH_FREE_RTOS
    taskENTER_CRITICAL();
    #else
    uint32_t interrupts = save_and_disable_interrupts();
    #endif

    /*
        Programming a flash page effectively changes some of the bits from one to zero.
        The only way to change a zero bit back to one is to "erase" the whole sector that
        the page resides in. So you may need to make sure you have called flash_range_erase
        before calling flash_range_program.
        https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#function-documentation65
    */

    memcpy(erase_buffer, _flash_offset_addr, _flash_user_erase_span);
    flash_range_erase(_flash_offset, _flash_user_erase_span);
    memcpy(erase_buffer, data, _flash_user_erase_span);
    flash_range_program(_flash_offset, erase_buffer, _flash_user_erase_span);

     // exit critical
    #ifdef _USER_FLASH_FREE_RTOS
    taskEXIT_CRITICAL();
    #else
    restore_interrupts(interrupts);
    #endif
}


void UserFlashBase::base_flash_load(void* data)
{
    // enter critical
    #ifdef _USER_FLASH_FREE_RTOS
    taskENTER_CRITICAL();
    #else
    uint32_t interrupts = save_and_disable_interrupts();
    #endif
    // critical section
    memcpy(data, _flash_offset_addr, _flash_user_size);
    // exit critical
    #ifdef _USER_FLASH_FREE_RTOS
    taskEXIT_CRITICAL();
    #else
    restore_interrupts(interrupts);
    #endif
}

uint32_t UserFlashBase::_moving_user_flash_end = 0;