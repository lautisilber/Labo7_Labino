/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "bme280_class.hpp"
#include "stepper.hpp"

// Stepper stepper(15, 14, 13, 12, STEPPER_HALF, true, 0, 10000);

// inline bool bme280_init_main()
// {
//     return bme280_handler_init(NULL, false, 0, NULL);
// }

// inline const struct bme280_data *bme280_read_main()
// {
//     return bme280_handler_get_sensor_data();
// }

BME280_I2C bme280;
Stepper stepper(true, 0, 1000);

int main() {
    stdio_init_all();
// #ifndef PICO_DEFAULT_LED_PIN
// #warning blink example requires a board with a regular LED
// #else
//     const uint LED_PIN = PICO_DEFAULT_LED_PIN;
//     gpio_init(LED_PIN);
//     gpio_set_dir(LED_PIN, GPIO_OUT);
//     while (true) {
//         gpio_put(LED_PIN, 1);
//         sleep_ms(250);
//         gpio_put(LED_PIN, 0);
//         sleep_ms(250);
//     }
// #endif

    // stepper.begin();

    stepper.begin();
    while (!bme280.begin())
    {
        sleep_ms(500);
    }


    bool alternate = false;
    for (;;)
    {
        const struct bme280_data *data;
        if (bme280.get_sensor_data(data))
        {
            printf("temp: %.2f, press: %.2f, hum: %.2f\n", data->temperature, data->pressure, data->humidity);
        }

        stepper.move_steps_async(1000 + 500 * (2*alternate - 1));
        alternate != alternate;

        sleep_ms(5000);
    }
}
