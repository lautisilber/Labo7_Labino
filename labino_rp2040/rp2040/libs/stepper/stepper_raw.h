#ifndef STEPPER_RAW_H
#define STEPPER_RAW_H

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

#include "pico/stdlib.h"

enum StepType
{
    STEPPER_WAVE,
    STEPPER_NORMAL,
    STEPPER_HALF
};

bool stepper_make_steps_clockwise(uint pin_1, uint pin_2, uint pin_3, uint pin_4,
                                     enum StepType step_type, uint32_t steps);
int32_t stepper_make_steps_anticlockwise(uint pin_1, uint pin_2, uint pin_3, uint pin_4,
                                     enum StepType step_type, uint32_t steps);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* STEPPER_RAW_H  */
