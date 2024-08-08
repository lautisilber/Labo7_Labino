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


#define USER_FLASH_SIZE              2048
#define USER_FLASH_SAVE_BEGIN_ADRESS (PICO_FLASH_SIZE_BYTES - USER_FLASH_SIZE)
#define USER_FLASH_PAGE_SIZE         256

bool user_flash_save(uint32_t location, const uint8_t *data, size_t length);
bool user_flash_load(uint32_t location, uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* USER_FLASH_H */