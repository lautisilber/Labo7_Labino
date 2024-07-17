#include "user_flash.h"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <string.h>

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

static uint32_t _n_users = 0;
static uint32_t _user_sizes[FLASH_USER_MAX_USERS] = {0};

static inline bool _is_user_index_valid(uint32_t user_index)
{
    return user_index < FLASH_USER_MAX_USERS;
}

static inline bool _is_user_registered(uint32_t user_index)
{
    return user_index < _n_users && _is_user_index_valid(user_index);
}

static uint32_t _get_user_address_offset(uint32_t user_index)
{
    if (!_is_user_index_valid(user_index)) return 0;
    uint32_t offset = 0;
    for (size_t i = 0; i < _n_users; i++)
    {
        offset += _user_sizes[i];
    }
    return offset;
}

bool register_new_user(uint32_t *user_index, uint32_t length)
{
    if (_n_users >= FLASH_USER_MAX_USERS - 1) return false;
    uint32_t user_offset = _get_user_address_offset(_n_users);
    if (user_offset + length > FLASH_USER_SAVE_BYTES_SIZE) return false;

    _user_sizes[_n_users] = length;
    *user_index = _n_users++;
    return true;
}

bool flash_user_save_raw(uint32_t location, const uint8_t *data, size_t length)
{
    const uint32_t offset = location + FLASH_USER_SAVE_BEGIN_ADRESS;
    if (offset < FLASH_USER_SAVE_BEGIN_ADRESS || offset + length >= PICO_FLASH_SIZE_BYTES)
        return false;

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_program(offset, data, length);
    restore_interrupts(interrupts);

    return true;
}

bool flash_user_load_raw(uint32_t location, uint8_t *data, size_t length)
{
    const uint32_t offset = location + FLASH_USER_SAVE_BEGIN_ADRESS;
    if (offset < FLASH_USER_SAVE_BEGIN_ADRESS || offset + length >= PICO_FLASH_SIZE_BYTES)
        return false;

    // Compute the memory-mapped address, remembering to include the offset for RAM
    uint8_t *addr = XIP_BASE + offset;
    
    uint32_t interrupts = save_and_disable_interrupts();
    memcpy(data, addr, length);
    restore_interrupts(interrupts);

    return true;
}

bool flash_user_save(uint32_t user_index, const uint8_t *data)
{
    if (!_is_user_registered(user_index)) return false;
    return flash_user_save_raw(_get_user_address_offset(user_index), data, _user_sizes[user_index]);
}

bool flash_user_load(uint32_t user_index, uint8_t *data)
{
    if (!_is_user_registered(user_index)) return false;
    return flash_user_load_raw(_get_user_address_offset(user_index), data, _user_sizes[user_index]);
}

#ifdef __cplusplus
}
#endif /* End of CPP guard */