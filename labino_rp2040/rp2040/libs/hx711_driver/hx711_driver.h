#ifndef HX711_DRIVER_H
#define HX711_DRIVER_H

#include "pico/stdlib.h"

// ported from https://github.com/bogde/HX711, https://github.com/nimaltd/HX711, https://github.com/endail/hx711-pico-c

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

enum HX711Gain
{
    // the numbers are the extra cycles needed to set each gain up
    A128 = 1,
    A64 = 3,
    B32 = 2
};

struct HX711
{
    uint dout, pd_sck;
    enum HX711Gain gain;
};

bool hx711_is_ready(const struct HX711 *hx);
bool hx711_wait_ready(const struct HX711 *hx, uint32_t timeout_ms);

void hx711_power_down(const struct HX711 *hx);
void hx711_power_down_blocking(const struct HX711 *hx);
void hx711_power_up(const struct HX711 *hx);

bool hx711_read(const struct HX711 *hx, int32_t *result, uint32_t timeout_ms);


#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* HX711_DRIVER_H */