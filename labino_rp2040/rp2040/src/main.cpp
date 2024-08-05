#include "pico/stdlib.h"
#include "pico.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "bme280_class.hpp"
#include "stepper.hpp"
#include "hx711_mux.hpp"
#include "analog_sensor.hpp"


int __attribute__ ((noreturn)) main() {
    stdio_init_all();
    for (;;)
    {
        tight_loop_contents();
    }
    __builtin_unreachable();
}

