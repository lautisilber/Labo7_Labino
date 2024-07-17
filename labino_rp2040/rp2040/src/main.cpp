#include "pico/stdlib.h"
#include "pico.h"
#include "bme280_class.hpp"
#include "stepper.hpp"
#include "hx711_mux.hpp"

int __attribute__ ((noreturn)) main() {
    for (;;)
    {
        tight_loop_contents();
    }
    __builtin_unreachable();
}