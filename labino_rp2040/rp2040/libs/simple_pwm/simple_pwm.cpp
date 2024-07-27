#include "simple_pwm.hpp"

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <math.h>

#define CLAMP(x, min, max) MAX(MIN(x, max), min)

SimplePWM::SimplePWM(uint pin, uint32_t period_us)
    : _pin(pin), _pwm_slice_num(pwm_gpio_to_slice_num(pin)), _pwm_channel(pwm_gpio_to_channel(pin))
{
    set_period_us(period_us);
}

inline uint32_t SimplePWM::period_2_cycles_per_period(uint32_t period_us)
{
    // period / internal_clock_pulse
    const uint32_t internal_clock_pulse_us = 8;
    return MAX(period_us, internal_clock_pulse_us) / (internal_clock_pulse_us);
}

inline uint16_t SimplePWM::pulse_width_2_set_point(uint32_t pulse_width_us) const
{
    return MIN((uint16_t)period_2_cycles_per_period(pulse_width_us), _cycles_per_period);
}

inline float SimplePWM::pulse_width_2_duty_cycle(uint32_t pulse_width_us) const
{
    return ((float)pulse_width_us) / ((float)_period_us);
}

inline uint32_t SimplePWM::duty_cycle_2_pulse_width_us(float duty_cycle) const
{
    duty_cycle = CLAMP(duty_cycle, 0.0, 1.0);
    return (uint32_t)round((float)_period_us / duty_cycle);
}

void SimplePWM::begin()
{
    gpio_set_function(_pin, GPIO_FUNC_PWM);
    pwm_set_wrap(_pin, _cycles_per_period);
}

void SimplePWM::set_period_us(uint32_t period_us)
{
    _period_us = period_us;
    _cycles_per_period = period_2_cycles_per_period(period_us);
}

uint32_t SimplePWM::set_pulse_width_us(uint32_t width_us)
{
    _pulse_width_us = CLAMP(width_us, 0, _period_us);
    const uint16_t level = pulse_width_2_set_point(_pulse_width_us);
    pwm_set_chan_level(_pwm_slice_num, _pwm_channel, level);
    return _pulse_width_us;
}


float SimplePWM::set_duty_cycle(float duty_cycle)
{
    uint32_t pulse_width_us = duty_cycle_2_pulse_width_us(duty_cycle);
    set_pulse_width_us(pulse_width_us);
    return CLAMP(duty_cycle, 0.0, 1.0);
}

void SimplePWM::enable(bool state)
{
    if (state != _enabled)
    {
        pwm_set_enabled(_pwm_slice_num, state);
        _enabled = state;
    }
}

uint32_t SimplePWM::get_period_us() const
{
    return _period_us;
}

uint32_t SimplePWM::get_pulse_width_us() const
{
    return _pulse_width_us;
}

uint32_t SimplePWM::get_duty_cycle() const
{
    return pulse_width_2_duty_cycle(_pulse_width_us);
}