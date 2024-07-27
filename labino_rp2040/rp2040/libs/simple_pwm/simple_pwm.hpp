#ifndef SIMPLE_PWM_HPP
#define SIMPLE_PWM_HPP


#include "pico/stdlib.h"


class SimplePWM
{
/*
    set point is the number of cycles (has to be <= to cycles per period)
    that the signal will be high, before being set to low for the rest of the period
*/
private:
    const uint _pin;
    const uint _pwm_slice_num;
    const uint _pwm_channel;

    bool _enabled = false;

    uint32_t _cycles_per_period, _period_us; // is correlated to _cycles_per_period
    uint32_t _pulse_width_us = 0;

private:
    static inline uint32_t period_2_cycles_per_period(uint32_t period_us);
    inline uint16_t pulse_width_2_set_point(uint32_t pulse_width_us) const;
    inline float pulse_width_2_duty_cycle(uint32_t pulse_width_us) const;
    inline uint32_t duty_cycle_2_pulse_width_us(float duty_cycle) const;

public:
    SimplePWM(uint pin, uint32_t period_us);

    void begin();
    void set_period_us(uint32_t period_us);
    uint32_t set_pulse_width_us(uint32_t width_us); // returns the actual pulse_width set (clamped)
    float set_duty_cycle(float duty_cylce);
    void enable(bool state);

    uint32_t get_period_us() const;
    uint32_t get_pulse_width_us() const;
    uint32_t get_duty_cycle() const;
};




#endif /* SIMPLE_PWM_HPP */