/*! CPP guard */

#include "stepper_raw.h"

#include <stdbool.h>

#define STEPPER_RAW_STEP_SLEEP_MS 1

#ifdef __cplusplus
extern "C" {
#endif

// step patterns

static const bool step_wave [4][4] =
{
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
};

static const bool step_normal [4][4] =
{
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1}
};

static const bool step_half [8][4] =
{
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};
//

static bool get_step_info(enum StepType step_type, uint8_t *n_step_variants, const bool (*stepTable)[4])
{
    switch (step_type) {
        STEPPER_WAVE:
            n_step_variants = 4;
            stepTable = step_wave;
            break;
        STEPPER_NORMAL:
            n_step_variants = 4;
            stepTable = step_normal;
            break;
        STEPPER_HALF:
            n_step_variants = 8;
            stepTable = step_half;
            break;
        default:
            return false;
    }
    return true;
}

bool stepper_make_steps_clockwise(uint pin_1, uint pin_2, uint pin_3, uint pin_4,
                                     enum StepType step_type, uint32_t steps)
{
    uint8_t n_step_variants;
    const bool (*step_table)[4];
    uint32_t mask = get_gpio_mask(pin_1, pin_2, pin_3, pin_4);

    if (!get_step_info(step_type, &n_step_variants, step_table))
        return false;

    for (uint32_t n = 0; n < steps; n++)
    {
        for (int8_t i = 0; i < n_step_variants; i++)
        {
            gpio_put(pin_1, step_table[i][0]);
            gpio_put(pin_2, step_table[i][1]);
            gpio_put(pin_3, step_table[i][2]);
            gpio_put(pin_4, step_table[i][3]);
            sleep_ms(STEPPER_RAW_STEP_SLEEP_MS);
        }
    }

    return true;
}

int32_t stepper_make_steps_anticlockwise(uint pin_1, uint pin_2, uint pin_3, uint pin_4,
                                     enum StepType step_type, uint32_t steps)
{
    uint8_t n_step_variants;
    const bool (*step_table)[4];
    uint32_t mask = get_gpio_mask(pin_1, pin_2, pin_3, pin_4);

    if (!get_step_info(step_type, &n_step_variants, step_table))
        return 0;

    for (uint32_t n = 0; n < steps; n++)
    {
        for (int8_t i = n_step_variants-1; i >= 0; i--)
        {
            gpio_put(pin_1, step_table[i][0]);
            gpio_put(pin_2, step_table[i][1]);
            gpio_put(pin_3, step_table[i][2]);
            gpio_put(pin_4, step_table[i][3]);
            sleep_ms(STEPPER_RAW_STEP_SLEEP_MS);
        }
    }
}

#ifdef __cplusplus
}
#endif /* End of CPP guard */
