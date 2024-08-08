#include "pico/stdlib.h"
#include <stdio.h>
#include "user_flash_class.hpp"
#include <string.h>
#include "hardware/structs/rosc.h"

#define DATA_LENGTH 128
#define ENDSTDIN	255
#define CR		    13

const char *data_sets[2] = 
{
    "hola",
    "chau"
};

bool get_random_bit() {
    return rosc_hw->randombit;
}

struct SaveData { char data[DATA_LENGTH]; };
/*
struct SavableClass : public UserFlashBase
{
private:
    SaveData sd;

public:
    SavableClass() : UserFlashBase(sizeof(SaveData))
    {}

    void get_data()
    {
        memset(sd.data, 0, DATA_LENGTH);
        // char chr;
        // size_t i;

        // printf("Input new data: ");
        
        // while (true) {

        //     chr = getchar_timeout_us(0);
        //     while(chr != ENDSTDIN)
        //     {
        //         sd.data[i++] = chr;
        //         if(chr == CR || i == (DATA_LENGTH - 1))
        //         {
        //             sd.data[i] = 0;	//terminate string
        //             i = 0;		//reset string buffer pointer
        //             break;
        //         }

        //         chr = getchar_timeout_us(0);
        //     }
        // }

        memcpy(sd.data, data_sets[get_random_bit()], DATA_LENGTH);

        printf("Written to RAM - ");
        print();
    }

    void print()
    {
        printf("Data in RAM is '%s'\n", sd.data);
    }

    bool save()
    {
        printf("Saving to flash... ");
        bool res = base_flash_save(&sd);
        printf("%u\n", res);
        return res;
    }

    bool load()
    {
        printf("Loading from flash... ");
        bool res = base_flash_load(&sd);
        printf("%u\n", res);
        return res;
    }
};


SavableClass sc;*/

void test()
{
    // sc.load();
    // sc.print();
    // sc.get_data();
    // sc.save();

    
}

int main()
{
    stdio_init_all();

    test();

    for (;;)
    {
        sleep_ms(1000);
    }
    __builtin_unreachable();
}