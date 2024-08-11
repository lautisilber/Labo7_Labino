#ifndef USER_FLASH_CLASS_HPP
#define USER_FLASH_CLASS_HPP

#include <pico/stdlib.h>

#define USER_FLASH_SIZE              4096
#define USER_FLASH_LOCATION_BEGIN    (PICO_FLASH_SIZE_BYTES - USER_FLASH_SIZE)

class UserFlashBase
{
private:
    static uint32_t _moving_user_flash_end;
protected:
    const uint32_t _flash_offset;
    uint8_t *const _flash_offset_addr; // Compute the memory-mapped address, remembering to include the offset for RAM
    const uint32_t _flash_user_size;
    const uint32_t _flash_user_page_span;    // number of bytes required for the data to fit in the least amount of FLASH_PAGE_SIZE possible
    const uint32_t _flash_user_erase_span;   // number of bytes that are both multiples of FLASH_SECTOR_SIZE and greater than _flash_user_page_span

    UserFlashBase(uint32_t flash_user_size);

    void base_flash_save(const void* data);
    void base_flash_load(void* data);
};


#endif /* USER_FLASH_CLASS_HPP */