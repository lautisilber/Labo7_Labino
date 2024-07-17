#ifndef USER_FLASH_CLASS_HPP
#define USER_FLASH_CLASS_HPP

#include "user_flash.h"
#include "panic.h"


class UserFlashBase
{
protected:
    uint32_t _flash_user_index;
    const uint32_t _flash_user_size;

    UserFlashBase(uint32_t flash_user_size) : _flash_user_size(flash_user_size)
    {
        if (!register_new_user(&_flash_user_index, _flash_user_size))
        {
            panic("Couldn't find space for new user in flash!");
        }
    }

    inline bool base_flash_save(const void* data)
    {
        return flash_user_save(_flash_user_index, (const uint8_t*)data);
    }

    inline bool base_flash_load(void* data)
    {
        return flash_user_load(_flash_user_index, (uint8_t*)data);
    }
};


#endif /* USER_FLASH_CLASS_HPP */