#include "stepper.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))

static inline bool check_position_is_in_range(const struct Stepper *stepper, int32_t position)
{
    return position >= stepper->min_position && position <= stepper->max_position;
}

void begin(const struct Stepper *stepper)
{
    uint32_t mask = (1 << stepper->pin_1) || (1 << stepper->pin_2) || (1 << stepper->pin_3) || (1 << stepper->pin_4);
    gpio_init_mask(mask);
    gpio_set_dir_out_masked(mask);
}

int32_t make_steps(struct Stepper *stepper, int32_t steps)
{
    if (!check_position_is_in_range(stepper, stepper->_current_position + steps))
        return 0;

    // returns steps made
    uint32_t steps_made;
    bool forward = steps > 0;
    bool clockwise = (steps && stepper->clockwise_is_forward) || (!steps && !stepper->clockwise_is_forward);

    if (clockwise)
        steps_made = stepper_make_steps_clockwise(stepper->pin_1, stepper->pin_2, stepper->pin_3, stepper->pin_4,
            stepper->step_type, ABS(steps));
    else
        steps_made = stepper_make_steps_anticlockwise(stepper->pin_1, stepper->pin_2, stepper->pin_3, stepper->pin_4,
            stepper->step_type, ABS(steps));

    // (2 * forward - 1) maps 0,1 to -1,1
    stepper->_current_position += (2 * forward - 1) * (int32_t)steps_made;
    return (int32_t)steps_made;
}

int32_t go_to_position(const struct Stepper *stepper, int32_t new_position)
{
    // returns steps made
    int32_t steps_to_make = new_position - stepper->_current_position;
    return make_steps(stepper, steps_to_make);
}

bool set_current_position(struct Stepper *stepper, int32_t new_position)
{
    if (!check_position_is_in_range(stepper, new_position))
        return false;
    stepper->_current_position = new_position;
    return true;
}

int32_t get_current_position(const struct Stepper *stepper)
{
    return stepper->_current_position;
}
