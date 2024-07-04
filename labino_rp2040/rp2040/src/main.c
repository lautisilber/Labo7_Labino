/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "bme280_defs.h"
#include "pico/stdlib.h"
#include "bme280_handler.h"
#include "pico/time.h"

inline bool bme280_init_main()
{
    return bme280_handler_init(NULL, false, 0, NULL);
}

inline const struct bme280_data *bme280_read_main()
{
    return bme280_handler_get_sensor_data();
}


int main() {
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
    bool res = false;
    while (!res)
    {
        res = bme280_init_main();
        sleep_ms(50);
    }

    uint32_t min_delay_ms;
    bme280_handler_cal_meas_delay(&min_delay_ms, false);

    while (true)
    {
        const struct bme280_data *data = bme280_read_main();
        if (data)
        {
            printf("temp: %.2f, press: %.2f, hum: %.2f\n", data->temperature, data->pressure, data->humidity);
        }
        sleep_ms(2*min_delay_ms);
    }
}
