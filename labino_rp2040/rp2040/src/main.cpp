#include "pico/stdlib.h"
// #include "pico.h"
// #include <stdio.h>
// #include "bme280_class.hpp"
// #include "stepper.hpp"
// #include "hx711_mux.hpp"
// #include "analog_sensor.hpp"

/* Scheduler include files. */
// #include "FreeRTOS.h"
// #include "task.h"
// #include "queue.h"
// #include "semphr.h"
// #include "event_groups.h"
// #include "timers.h"

// HX711Mux hx711s();

int __attribute__ ((noreturn)) main() {
    stdio_init_all();
    for (;;)
    {
        tight_loop_contents();
    }
    __builtin_unreachable();
}

