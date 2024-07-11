#include "hx711_driver.h"

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

bool hx711_is_ready(const struct HX711 *hx)
{
    // low means ready
    return !gpio_get(hx->dout);
}

bool hx711_wait_ready(const struct HX711 *hx, uint32_t timeout_ms)
{
    bool ready = hx711_is_ready(hx);
    absolute_time_t timeout_stamp = make_timeout_time_ms(timeout_ms);
    while (absolute_time_diff_us(get_absolute_time(), timeout_stamp) < 0)
    {
        sleep_ms(100);
        ready = hx711_is_ready(hx);
    }
    return ready;
}

void hx711_power_down(const struct HX711 *hx)
{
    gpio_put(hx->pd_sck, true);
}

void hx711_power_down_blocking(const struct HX711 *hx)
{
    hx711_power_down(hx);
    sleep_us(60);
}

void hx711_power_up(const struct HX711 *hx)
{
    gpio_put(hx->pd_sck, false); // pull low to indicate we want to start measuring
}

bool hx711_read(const struct HX711 *hx, int32_t *result, uint32_t timeout_ms)
{
    hx711_power_up(hx);
    if (!hx711_wait_ready(hx, timeout_ms)) return false;

    uint32_t data = 0;

    for (uint8_t i = 0; i < 24 ; i++)
    {
        // pulse
        gpio_put(hx->pd_sck, true);
        sleep_us(1);
        gpio_put(hx->pd_sck, false);
        sleep_us(1);

        data = data << 1;
        if (gpio_get(hx->dout))
            ++data;
    }

    for (uint8_t i = 0; i < hx->gain; i++)
    {
        // pulse
        gpio_put(hx->pd_sck, true);
        sleep_us(1);
        gpio_put(hx->pd_sck, false);
        sleep_us(1);
    }

    data = data ^ 0x800000;
    return data;
}


#ifdef __cplusplus
}
#endif /* End of CPP guard */