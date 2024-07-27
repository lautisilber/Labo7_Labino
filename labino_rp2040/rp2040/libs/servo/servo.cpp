#include "servo.hpp"

#include "pico/time.h"
#include "debug_helper.h"
#include "utils.hpp"
#include <hardware/sync.h>


inline bool Servo::_is_angle_valid(uint8_t angle)
    {
        return angle >= SERVO_DEFAULT_MIN_ANGLE && angle <= SERVO_DEFAULT_MAX_ANGLE;
    }

    inline uint32_t Servo::_angle_2_pulse_width_us(uint8_t angle)
    {
        return map_2_range<uint32_t, float>(angle, _min_angle, _max_angle, _min_pulse_width_us, _max_pulse_width_us);
    }

    inline void Servo::_go_to_angle_instantaneously_unsafe(uint8_t angle)
    {
        // note that it is not assured that the pulse width is changed to 
        const uint32_t pulse_width_us = _angle_2_pulse_width_us(angle);
        set_pulse_width_us(pulse_width_us);
    }

    bool Servo::asnyc_callback(repeating_timer_t *rt)
    {
        Servo *servo = (Servo *)rt->user_data;
        AsyncData *async_data = &servo->async_data;
        
        if (++async_data->current_step < async_data->n_total_steps)
        {
            async_data->last_pulse_width_us += async_data->step_size_us;
            servo->set_pulse_width_us(async_data->last_pulse_width_us);
            return true; // repeat
        }
        else
        {
            servo->set_pulse_width_us(async_data->end_pulse_width_us);
            if (servo->async_end_callback) servo->async_end_callback(async_data->start_angle, async_data->end_angle);
            servo->_running_flag = false;
            servo->_curr_angle = async_data->end_angle;
            return false; // repeat
        }
    }

    Servo::Servo(uint pin)
        : SimplePWM(pin, SERVO_DEFAULT_PWM_PERIOD_US), _min_angle(SERVO_DEFAULT_MIN_ANGLE), _max_angle(SERVO_DEFAULT_MAX_ANGLE),
          _min_pulse_width_us(SERVO_DEFAULT_MIN_PULSE_WIDTH_US), _max_pulse_width_us(SERVO_DEFAULT_MAX_PULSE_WIDTH_US)
    {}

    Servo::Servo(uint pin, uint32_t delay_us, uint32_t step_pulse_width_us,
          uint32_t min_angle_pulse_width_us, uint32_t max_angle_pulse_width_us,
          uint8_t _min_angle, uint8_t _max_angle)
        : SimplePWM(pin, SERVO_DEFAULT_PWM_PERIOD_US), _delay_us(delay_us), _step_pulse_width_us(step_pulse_width_us),
          _min_pulse_width_us(min_angle_pulse_width_us), _max_pulse_width_us(max_angle_pulse_width_us),
          _min_angle(SERVO_DEFAULT_MIN_ANGLE), _max_angle(SERVO_DEFAULT_MAX_ANGLE)
    {}

    void Servo::begin(uint8_t init_angle)
    {
        SimplePWM::begin();
        if (!_is_angle_valid(init_angle))
        {
            WARN_PRINTFLN("Servo init angle of %u is not inside range [%u, %u]", init_angle, SERVO_DEFAULT_MIN_ANGLE, SERVO_DEFAULT_MAX_ANGLE);
            init_angle = CLAMP(init_angle, SERVO_DEFAULT_MIN_ANGLE, SERVO_DEFAULT_MAX_ANGLE); // clamp
        }
        _curr_angle = init_angle;
    }

    bool Servo::go_to_angle_blocking(uint8_t angle)
    {
        if (!_is_angle_valid(angle))
        {
            WARN_PRINTFLN("go_to_angle_blocking angle parameter of %u is not inside range [%u, %u]", angle, SERVO_DEFAULT_MIN_ANGLE, SERVO_DEFAULT_MAX_ANGLE);
            return false;
        }

        if (is_running()) return false;
        _running_flag = true;


        const uint32_t end_pulse_width_us = _angle_2_pulse_width_us(angle);
        const int8_t direction = sign(end_pulse_width_us - get_pulse_width_us());
        const uint32_t n_steps = lerp(get_pulse_width_us(), end_pulse_width_us, _step_pulse_width_us);
        const uint32_t step = ((uint32_t)direction * _step_pulse_width_us);

        enable(true);

        for (uint32_t i = 0; i < n_steps; i++)
        {
            set_pulse_width_us(get_pulse_width_us() + step);
            sleep_us(_delay_us);
        }
        set_pulse_width_us(end_pulse_width_us);

        _curr_angle = angle;
        _running_flag = false;

        return true;
    }

    bool Servo::go_to_angle_async(uint8_t angle, servo_end_rotation_cb_t cb)
    {
        if (is_running()) return false;

        async_end_callback = cb;

        async_data.last_pulse_width_us = get_pulse_width_us();
        async_data.end_pulse_width_us = _angle_2_pulse_width_us(angle);
        async_data.n_total_steps = lerp(get_pulse_width_us(), async_data.end_pulse_width_us, _step_pulse_width_us);
        async_data.step_size_us = _step_pulse_width_us;
        async_data.current_step = 0;
        async_data.start_angle = _curr_angle;
        async_data.end_angle = angle;


        // not necessary, because we have already stated that the async is not running
        // uint32_t interrupts = save_and_disable_interrupts();
        _running_flag = true;
        // restore_interrupts(interrupts);

        bool res = add_repeating_timer_us(_delay_us, asnyc_callback, this, _async_timer_in_use);
        if (!res) _running_flag = false; // to avoid soft locks
        return res;
    }

    inline bool Servo::is_running()
    {
        bool local_running_flag;
        uint32_t interrupts = save_and_disable_interrupts();
        local_running_flag = _running_flag;
        restore_interrupts(interrupts);
        return local_running_flag;
    }

    uint8_t Servo::get_angle() const
    {
        bool local_curr_angle;
        uint32_t interrupts = save_and_disable_interrupts();
        local_curr_angle = _curr_angle;
        restore_interrupts(interrupts);
        return local_curr_angle;
    }