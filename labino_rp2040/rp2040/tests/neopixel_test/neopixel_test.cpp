#include "pico/stdlib.h"
#include <stdio.h>
#include "NeoPixel.hpp"

#define XIAO_N_NEOPIXELS   1
#define XIAO_NEOPIXEL_PIN  12
#define XIAO_NEOPIXEL_TYPE NEO_GRB + NEO_KHZ800
NeoPixel neopixel(XIAO_N_NEOPIXELS, XIAO_NEOPIXEL_PIN, XIAO_NEOPIXEL_TYPE);

int main()
{
    stdio_init_all();
    printf("Testing neopixel\n");

    neopixel.begin();

    uint8_t r, g, b;

    for (;;)
    {
        for (uint8_t i; i < 4 ;i++) {
            r = (i%3)*150;
            g = ((i+1)%3)*150;
            b = ((i+2)%3)*150;
            neopixel.clear(); // Set all pixel colors to 'off'
            neopixel.setPixelColor(0, neopixel.Color(r, g, b));
            neopixel.show();
            printf("Setting color to (%u, %u, %u)\n", r, g, b);
            sleep_ms(500);
        }
    }
}