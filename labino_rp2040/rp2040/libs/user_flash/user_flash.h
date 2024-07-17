#ifndef USER_FLASH_H
#define USER_FLASH_H

#include <pico/stdlib.h>

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

/*
    PICO_FLASH_SIZE_BYTES # The total size of the RP2040 flash, in bytes
    FLASH_SECTOR_SIZE     # The size of one sector, in bytes (the minimum amount you can erase)
    FLASH_PAGE_SIZE       # The size of one page, in bytes (the mimimum amount you can write)
*/

#define FLASH_USER_SAVE_BYTES_SIZE   (16 * 20)
#define FLASH_USER_SAVE_N_SECTORS    (FLASH_SECTOR_SIZE / FLASH_USER_SAVE_BYTES_SIZE + (FLASH_SECTOR_SIZE % FLASH_USER_SAVE_BYTES_SIZE > 0))
#define FLASH_USER_SAVE_BEGIN_ADRESS (PICO_FLASH_SIZE_BYTES - (FLASH_USER_SAVE_N_SECTORS * FLASH_SECTOR_SIZE))
#define FLASH_USER_MAX_USERS 16

bool register_new_user(uint32_t *user_index, uint32_t length);

bool flash_user_save_raw(uint32_t location, const uint8_t *data, size_t length);
bool flash_user_load_raw(uint32_t location, uint8_t *data, size_t length);

bool flash_user_save(uint32_t user_index, const uint8_t *data);
bool flash_user_load(uint32_t user_index, uint8_t *data);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* USER_FLASH_H */