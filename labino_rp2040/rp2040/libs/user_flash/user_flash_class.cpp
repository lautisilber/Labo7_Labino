#include "user_flash_class.hpp"

#include "user_panic.h"

UserFlashBase::UserFlashBase(uint32_t flash_user_size)
    : _flash_user_size(flash_user_size), _flash_offset(UserFlashBase::_moving_user_flash_end)
{
    UserFlashBase::_moving_user_flash_end += _flash_user_size;
    if (UserFlashBase::_moving_user_flash_end % USER_FLASH_PAGE_SIZE != 0)
    {
        UserFlashBase::_moving_user_flash_end = USER_FLASH_PAGE_SIZE * ((UserFlashBase::_moving_user_flash_end / USER_FLASH_PAGE_SIZE) + 1);
    }
    if (UserFlashBase::_moving_user_flash_end >= USER_FLASH_SIZE)
    {
        USER_PANIC_PRE_MAIN("Couldn't assign user flash of size %u at starting location of %u due to overflow (total user flash size %u)", _flash_user_size, _flash_offset, USER_FLASH_SIZE);
    }
}


bool UserFlashBase::base_flash_save(const void* data)
{
    return user_flash_save(_flash_offset, (const uint8_t*)data, _flash_user_size);
}


bool UserFlashBase::base_flash_load(void* data)
{
    return user_flash_load(_flash_offset, (uint8_t*)data, _flash_user_size);
}

uint32_t UserFlashBase::_moving_user_flash_end = 0;