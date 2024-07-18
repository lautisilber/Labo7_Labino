#ifndef USER_FLASH_CLASS_HPP
#define USER_FLASH_CLASS_HPP

#include <pico/stdlib.h>
#include "user_flash.h"

class UserFlashBase
{
protected:
    uint32_t _flash_user_index;
    const uint32_t _flash_user_size;

    UserFlashBase(uint32_t flash_user_size);

    inline bool base_flash_save(const void* data);
    inline bool base_flash_load(void* data);
};


#endif /* USER_FLASH_CLASS_HPP */