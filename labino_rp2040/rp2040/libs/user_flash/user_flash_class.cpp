#include "user_flash_class.hpp"

#include "panic.h"

UserFlashBase::UserFlashBase(uint32_t flash_user_size)
    : _flash_user_size(flash_user_size)
{
    if (!register_new_user(&_flash_user_index, _flash_user_size))
    {
        panic_pre_main("Couldn't find space for new user in flash!");
    }
}


inline bool UserFlashBase::base_flash_save(const void* data)
{
    return flash_user_save(_flash_user_index, (const uint8_t*)data);
}


inline bool UserFlashBase::base_flash_load(void* data)
{
    return flash_user_load(_flash_user_index, (uint8_t*)data);
}