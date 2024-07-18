#ifndef ANALOG_SENSOR_HPP
#define ANALOG_SENSOR_HPP


#include <stdio.h>
#include "pico/stdlib.h"
// For ADC input:
#include "hardware/adc.h"
// #include "hardware/dma.h"
#include <math.h>

#include "panic.h"
#include "utils.hpp"

#define ANALOG_SENSOR_RESOLUTION float
// #define ANALOG_SENSOR_RESOLUTION double
#define ANALOG_SENSOR_MAX_ITERATIONS_BEFORE_USING_OFFLINE_MEAN_STDEV  256

inline bool is_pin_adc(uint pin)
{
    return pin > 25 && pin < 30;
}

inline uint8_t pin_2_adc_channel_unsafe(uint pin)
{
    return pin - 26;
}

bool get_adc_channel_from_pin(uint pin, uint8_t *adc_channel)
{
    if (!is_pin_adc(pin))
        return false;
    *adc_channel = pin_2_adc_channel_unsafe(pin);
    return true;
}

// bool get_pin_from_adc_channel(uint8_t adc_channel, uint *pin)
// {
//     if (adc_channel > 4)
//         return false;
//     *pin = adc_channel + 26;
//     return true;
// }


struct AnalogSensorAvgResult
{
    ANALOG_SENSOR_RESOLUTION mean, stdev;
    size_t count;
};


class AnalogSensorBase
{
protected:
    const uint8_t _adc_channel;
    const uint _pin;

protected: // statics
    static bool _adc_init_flag;
    static uint8_t _adc_used_channels_mask; // = 0b0000
    static uint8_t _selected_channel;

    static inline bool _is_channel_already_used(uint8_t adc_channel)
    {
        //   adc_channel is in [0,3]       adc_channel->mask
        return (adc_channel < 4)    &&    ((1 << adc_channel) & _adc_used_channels_mask);
    }

    static inline void _claim_adc_channel(uint8_t adc_channel)
    {
        if (adc_channel > 3) return;
        _adc_used_channels_mask |= (1 << adc_channel);
    }

private:
    void read_raw_avg_blocking_online(size_t n, ANALOG_SENSOR_RESOLUTION *mean, ANALOG_SENSOR_RESOLUTION *stdev)
    {
        uint16_t raws[n] = {0};
        for (size_t i = 0; i < n; i++)
            raws[i] = read_raw_single_blocking();
        calc_mean_stdev_online<uint16_t, ANALOG_SENSOR_RESOLUTION>(raws, n, mean, stdev);
    }

    void read_raw_avg_blocking_offline(size_t n, ANALOG_SENSOR_RESOLUTION *mean, ANALOG_SENSOR_RESOLUTION *stdev)
    {
        struct WelfordAggregate<ANALOG_SENSOR_RESOLUTION> welf_agg = {.count=0, .mean=0.0f, .m2=0.0f};
        int16_t raw;

        for (size_t i = 0; i < n; i++)
        {
            raw = read_raw_single_blocking();
            calc_mean_stdev_welford<int16_t, ANALOG_SENSOR_RESOLUTION>(&welf_agg, raw);
        }

        calc_mean_stdev_welford_finish<int16_t, ANALOG_SENSOR_RESOLUTION>(&welf_agg, mean, stdev);
    }

public:
    AnalogSensorBase(uint pin)
        : _pin(pin), _adc_channel(pin_2_adc_channel_unsafe(pin))
    {
        if (!is_pin_adc(_pin))
        {
            panic_pre_main("Can't construct AnalogSensor with pin %u, since it's not an ADC pin\n", _pin);
        }
        else if(_is_channel_already_used(_adc_channel))
        {
            panic_pre_main("Can't construct AnalogSensor with pin %u, since it's ADC channel (%u) is already in use by another AnalogSensor\n", _pin, _adc_channel);
        }
        _claim_adc_channel(_adc_channel);
        _selected_channel = 4;
    }

    void begin()
    {
        if (!_adc_init_flag)
        {
            adc_init();
            _adc_init_flag = true;
        }

        adc_gpio_init(_pin);
    }

    uint16_t read_raw_single_blocking()
    {
        if (_selected_channel != _adc_channel)
        {
            adc_select_input(_adc_channel);
            _selected_channel = _adc_channel;
        }
        return adc_read();
    }

    struct AnalogSensorAvgResult read_raw_avg_blocking(size_t n)
    {
        struct AnalogSensorAvgResult res = {.mean = 0, .stdev = 0, .count = n};
        if (n == 0) return res;
        else if (n == 1)
        {
            res.mean = (ANALOG_SENSOR_RESOLUTION)read_raw_single_blocking();
            res.stdev = 0;
        }
        else if (n <= ANALOG_SENSOR_MAX_ITERATIONS_BEFORE_USING_OFFLINE_MEAN_STDEV)
        {
            read_raw_avg_blocking_online(n, &res.mean, &res.stdev);
        }
        else
        {
            read_raw_avg_blocking_offline(n, &res.mean, &res.stdev);
        }
        return res;
    }
};

class IAnalogSensor : public AnalogSensorBase
{
public:
    IAnalogSensor(uint pin) : AnalogSensorBase(pin) {}
    virtual struct AnalogSensorAvgResult read_avg_blocking(size_t n) = 0;
};

class AnalogSensorLinear : IAnalogSensor
{
private:
    ANALOG_SENSOR_RESOLUTION _slope, _offset, _slope_error, _offset_error;
public:
    AnalogSensorLinear(uint pin, ANALOG_SENSOR_RESOLUTION slope, ANALOG_SENSOR_RESOLUTION slope_error, ANALOG_SENSOR_RESOLUTION offset, ANALOG_SENSOR_RESOLUTION offset_error)
        : IAnalogSensor(pin), _slope(slope), _offset(offset), _slope_error(slope_error), _offset_error(offset_error)
    {}
    struct AnalogSensorAvgResult read_avg_blocking(size_t n)
    {
        struct AnalogSensorAvgResult res = read_raw_avg_blocking(n);

        // v = r * s + o
        // v_err = sqrt( o_err^2 + s^2*r_err^2 + s_err^2*r^2 )
        // TODO: check that using res.mean and res.stdev twice like this works
        linear_error_propagation<ANALOG_SENSOR_RESOLUTION>(&res.mean, &res.stdev,
                                                           res.mean, res.stdev,
                                                           _slope, _slope_error,
                                                           _offset, _offset_error);

        return res;
    }
};

class AnalogSensorExponential : IAnalogSensor
{
private:
    const ANALOG_SENSOR_RESOLUTION _a, _b, _c, _d;
    const ANALOG_SENSOR_RESOLUTION _a_error, _b_error, _c_error, _d_error;
public:
    AnalogSensorExponential(uint pin, ANALOG_SENSOR_RESOLUTION a, ANALOG_SENSOR_RESOLUTION b, ANALOG_SENSOR_RESOLUTION c, ANALOG_SENSOR_RESOLUTION d,
                      ANALOG_SENSOR_RESOLUTION a_error, ANALOG_SENSOR_RESOLUTION b_error, ANALOG_SENSOR_RESOLUTION c_error, ANALOG_SENSOR_RESOLUTION d_error)
        : IAnalogSensor(pin), _a(a), _b(b), _c(c), _d(d),
          _a_error(a_error), _b_error(b_error), _c_error(c_error), _d_error(d_error)
    {}
    struct AnalogSensorAvgResult read_avg_blocking(size_t n)
    {
        struct AnalogSensorAvgResult res = read_raw_avg_blocking(n);

        // TODO: check that using res.mean and res.stdev twice like this works
        exponential_error_propagation<ANALOG_SENSOR_RESOLUTION>(&res.mean, &res.stdev,
                                                                res.mean, res.stdev,
                                                                _a, _a_error,
                                                                _b, _b_error,
                                                                _c, _c_error,
                                                                _d, _d_error);

        return res;
    }
};

class AnalogSensorLogarithmic : IAnalogSensor
{
private:
    const ANALOG_SENSOR_RESOLUTION _a, _b, _c, _d;
    const ANALOG_SENSOR_RESOLUTION _a_error, _b_error, _c_error, _d_error;
public:
    AnalogSensorLogarithmic(uint pin, ANALOG_SENSOR_RESOLUTION a, ANALOG_SENSOR_RESOLUTION b, ANALOG_SENSOR_RESOLUTION c, ANALOG_SENSOR_RESOLUTION d,
                      ANALOG_SENSOR_RESOLUTION a_error, ANALOG_SENSOR_RESOLUTION b_error, ANALOG_SENSOR_RESOLUTION c_error, ANALOG_SENSOR_RESOLUTION d_error)
        : IAnalogSensor(pin), _a(a), _b(b), _c(c), _d(d),
          _a_error(a_error), _b_error(b_error), _c_error(c_error), _d_error(d_error)
    {}
    struct AnalogSensorAvgResult read_avg_blocking(size_t n)
    {
        struct AnalogSensorAvgResult res = read_raw_avg_blocking(n);

        // TODO: check that using res.mean and res.stdev twice like this works
        logarithm_error_propagation<ANALOG_SENSOR_RESOLUTION>(&res.mean, &res.stdev,
                                                              res.mean, res.stdev,
                                                              _a, _a_error,
                                                              _b, _b_error,
                                                              _c, _c_error,
                                                              _d, _d_error);

        return res;
    }
};

#endif