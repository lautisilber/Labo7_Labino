#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

#include "user_flash_no_os.h"

void print_buf(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        printf("%02x", buf[i]);
        if (i % 16 == 15)
            printf("\n");
        else
            printf(" ");
    }
}

int main() {
    stdio_init_all();
    sleep_ms(1000);
    printf("begin\n");

    uint8_t random_data[FLASH_PAGE_SIZE];
    uint8_t buffer[FLASH_PAGE_SIZE] = {0};
    for (uint i = 0; i < FLASH_PAGE_SIZE; ++i)
        random_data[i] = rand() >> 16;

    printf("Generated random data:\n");
    print_buf(random_data, FLASH_PAGE_SIZE);

    printf("\nProgramming target region...");
    user_flash_save_no_os(0, random_data, FLASH_PAGE_SIZE);

    printf("\nReading back target region...");
    user_flash_load_no_os(0, buffer, FLASH_PAGE_SIZE);
    printf(" done.\nReading buffer:\n");
    print_buf(buffer, FLASH_PAGE_SIZE);

    
    bool mismatch = false;
    for (uint i = 0; i < FLASH_PAGE_SIZE; ++i) {
        if (random_data[i] != buffer[i])
            mismatch = true;
    }
    if (mismatch)
        printf("Programming failed!\n");
    else
        printf("Programming successful!\n");
}
