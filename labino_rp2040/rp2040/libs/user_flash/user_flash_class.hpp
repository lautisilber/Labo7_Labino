#ifndef USER_FLASH_CLASS_HPP
#define USER_FLASH_CLASS_HPP

#include <pico/stdlib.h>
#include "user_flash.h"

class UserFlashBase
{
private:
    static uint32_t _moving_user_flash_end;
protected:
    const uint32_t _flash_offset;
    const uint32_t _flash_user_size;

    UserFlashBase(uint32_t flash_user_size);

    bool base_flash_save(const void* data);
    bool base_flash_load(void* data);
};


#endif /* USER_FLASH_CLASS_HPP */