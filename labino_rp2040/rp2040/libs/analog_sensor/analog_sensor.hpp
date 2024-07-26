#ifndef ANALOG_SENSOR_HPP
#define ANALOG_SENSOR_HPP


#include "pico/stdlib.h"


#define ANALOG_SENSOR_RESOLUTION float
// #define ANALOG_SENSOR_RESOLUTION double
#define ANALOG_SENSOR_MAX_ITERATIONS_BEFORE_USING_OFFLINE_MEAN_STDEV  256




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

    static inline bool _is_channel_already_used(uint8_t adc_channel);
    static inline void _claim_adc_channel(uint8_t adc_channel);

private:
    void read_raw_avg_blocking_online(size_t n, ANALOG_SENSOR_RESOLUTION *mean, ANALOG_SENSOR_RESOLUTION *stdev);
    void read_raw_avg_blocking_offline(size_t n, ANALOG_SENSOR_RESOLUTION *mean, ANALOG_SENSOR_RESOLUTION *stdev);

public:
    AnalogSensorBase(uint pin);
    void begin();
    uint16_t read_raw_single_blocking();
    struct AnalogSensorAvgResult read_raw_avg_blocking(size_t n);
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
    AnalogSensorLinear(uint pin, ANALOG_SENSOR_RESOLUTION slope, ANALOG_SENSOR_RESOLUTION slope_error, ANALOG_SENSOR_RESOLUTION offset, ANALOG_SENSOR_RESOLUTION offset_error);
    struct AnalogSensorAvgResult read_avg_blocking(size_t n);
};

class AnalogSensorExponential : IAnalogSensor
{
private:
    const ANALOG_SENSOR_RESOLUTION _a, _b, _c, _d;
    const ANALOG_SENSOR_RESOLUTION _a_error, _b_error, _c_error, _d_error;
public:
    AnalogSensorExponential(uint pin, ANALOG_SENSOR_RESOLUTION a, ANALOG_SENSOR_RESOLUTION b, ANALOG_SENSOR_RESOLUTION c, ANALOG_SENSOR_RESOLUTION d,
                      ANALOG_SENSOR_RESOLUTION a_error, ANALOG_SENSOR_RESOLUTION b_error, ANALOG_SENSOR_RESOLUTION c_error, ANALOG_SENSOR_RESOLUTION d_error);
    struct AnalogSensorAvgResult read_avg_blocking(size_t n);
};

class AnalogSensorLogarithmic : IAnalogSensor
{
private:
    const ANALOG_SENSOR_RESOLUTION _a, _b, _c, _d;
    const ANALOG_SENSOR_RESOLUTION _a_error, _b_error, _c_error, _d_error;
public:
    AnalogSensorLogarithmic(uint pin, ANALOG_SENSOR_RESOLUTION a, ANALOG_SENSOR_RESOLUTION b, ANALOG_SENSOR_RESOLUTION c, ANALOG_SENSOR_RESOLUTION d,
                      ANALOG_SENSOR_RESOLUTION a_error, ANALOG_SENSOR_RESOLUTION b_error, ANALOG_SENSOR_RESOLUTION c_error, ANALOG_SENSOR_RESOLUTION d_error);
    struct AnalogSensorAvgResult read_avg_blocking(size_t n);
};

#endif