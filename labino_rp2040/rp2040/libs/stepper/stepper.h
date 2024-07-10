#ifndef STEPPER_H
#define STEPPER_H

#include "stepper_raw.h"

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

struct Stepper {
    uint pin_1, pin_2, pin_3, pin_4;
    enum StepType step_type;
    bool clockwise_is_forward;
    int32_t min_position, max_position;

    int32_t _current_position = 0;
};

void begin(const struct Stepper *stepper);
int32_t make_steps(struct Stepper *stepper, int32_t steps);
int32_t go_to_position(const struct Stepper *stepper, int32_t new_position);
bool set_current_position(struct Stepper *stepper, int32_t new_position);
int32_t get_current_position(const struct Stepper *stepper);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* STEPPER_H */
