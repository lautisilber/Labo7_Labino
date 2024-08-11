#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

#include "user_flash_no_os.h"

#include "pico/flash.h"

void print_buf(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        printf("%02x", buf[i]);
        if (i % 16 == 15)
            printf("\n");
        else
            printf(" ");
    }
}

#define LENGTH FLASH_PAGE_SIZE

int main() {
    stdio_init_all();
    sleep_ms(1000);
    printf("begin\n");

    uint8_t random_data[LENGTH];
    uint8_t data_buffer[LENGTH];
    for (uint i = 0; i < LENGTH; ++i)
        random_data[i] = rand() >> 16;

    printf("Generated random data:\n");
    print_buf(random_data, LENGTH);

    printf("\nProgramming target region...");
    user_flash_save_no_os(0, random_data, LENGTH);
    printf("\nDone. Read back target region:\n");
    print_buf(USER_FLASH_ADDRESS_POINTER_BEGIN, LENGTH);

    printf("Reading from target region...\n");
    user_flash_load_no_os(0, data_buffer, LENGTH);
    printf("\nDone. Read:\n");
    print_buf(data_buffer, LENGTH);
    

    bool mismatch = false;
    for (uint i = 0; i < LENGTH; ++i) {
        if (random_data[i] != data_buffer[i])
            mismatch = true;
    }
    if (mismatch)
        printf("Programming failed!\n");
    else
        printf("Programming successful!\n");
}
