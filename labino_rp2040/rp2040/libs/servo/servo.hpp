#ifndef SERVO_HPP
#define SERVO_HPP


#include "pico/stdlib.h"
#include "simple_pwm.hpp"

#define SERVO_DEFAULT_PWM_PERIOD_US 20*1000
#define SERVO_DEFAULT_MIN_ANGLE 1
#define SERVO_DEFAULT_MAX_ANGLE 179
#define SERVO_DEFAULT_MIN_PULSE_WIDTH_US 1000
#define SERVO_DEFAULT_MAX_PULSE_WIDTH_US 2000
#define SERVO_DEFAULT_DELAY_US 1000
#define SERVO_DEFAULT_STEP_PULSE_WIDTH_US 50


typedef void (*servo_end_rotation_cb_t)(uint8_t angle_start, uint8_t angle_end);

class Servo : public SimplePWM
{
private:
    uint32_t _delay_us, _step_pulse_width_us;
    const uint8_t _min_angle, _max_angle;
    const uint32_t _min_pulse_width_us, _max_pulse_width_us;

private:
    inline bool _is_angle_valid(uint8_t angle);
    inline uint32_t _angle_2_pulse_width_us(uint8_t angle);
    inline void _go_to_angle_instantaneously_unsafe(uint8_t angle);

private:
    // shouldn't be accessed from interrupt
    repeating_timer_t *_async_timer_in_use = nullptr;
public:
    // async related functions, not to be accessed by the user
    struct AsyncData
    {
        uint32_t n_total_steps, current_step;
        uint32_t last_pulse_width_us, end_pulse_width_us;
        uint32_t step_size_us;
        uint8_t start_angle, end_angle;
    };
    struct AsyncData async_data = {0};
    bool _running_flag = false;
    servo_end_rotation_cb_t async_end_callback = nullptr;
    uint8_t _curr_angle = 90;

    static bool asnyc_callback(repeating_timer_t *rt);

public:
    Servo(uint pin);
    Servo(uint pin, uint32_t delay_us=SERVO_DEFAULT_MIN_ANGLE, uint32_t step_pulse_width_us=SERVO_DEFAULT_STEP_PULSE_WIDTH_US,
          uint32_t min_angle_pulse_width_us=SERVO_DEFAULT_MIN_PULSE_WIDTH_US, uint32_t max_angle_pulse_width_us=SERVO_DEFAULT_MAX_PULSE_WIDTH_US,
          uint8_t _min_angle=SERVO_DEFAULT_MIN_ANGLE, uint8_t _max_angle=SERVO_DEFAULT_MAX_ANGLE);

    void begin(uint8_t init_angle=90);

    bool go_to_angle_blocking(uint8_t angle);
    bool go_to_angle_async(uint8_t angle, servo_end_rotation_cb_t cb=nullptr);

    inline bool is_running();
    uint8_t get_angle() const;

};

#endif /* SERVO_HPP */