#include "analog_sensor.hpp"

// For ADC input:
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <math.h>

#include "panic.h"
#include "utils.hpp"
#include "debug_helper.h"

#define _ANALOG_SENSOR_USE_DMA true

static inline bool is_pin_adc(uint pin)
{
    return pin > 25 && pin < 30;
}

static inline uint8_t pin_2_adc_channel_unsafe(uint pin)
{
    return pin - 26;
}

static bool get_adc_channel_from_pin(uint pin, uint8_t *adc_channel)
{
    if (!is_pin_adc(pin))
        return false;
    *adc_channel = pin_2_adc_channel_unsafe(pin);
    return true;
}

// static bool get_pin_from_adc_channel(uint8_t adc_channel, uint *pin)
// {
//     if (adc_channel > 4)
//         return false;
//     *pin = adc_channel + 26;
//     return true;
// }

static void read_adc_dma(uint16_t *arr, size_t n)
{
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        false     // Shift each sample to 8 bits when pushing to FIFO | TODO: check if true or false is better
    );

    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.
    adc_set_clkdiv(0);

    // TODO: check if this is necessary
    //sleep_ms(1000); // arming DMA

    // Set up the DMA to start transferring data as soon as it appears in FIFO
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16); // or DMA_SIZE_8
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(dma_chan, &cfg,
        arr,            // dst
        &adc_hw->fifo,  // src
        n,              // transfer count
        true            // start immediately
    );

    adc_run(true); // starting capture

    // Once DMA finishes, stop any new conversions from starting, and clean up
    // the FIFO in case the ADC was still mid-conversion.
    dma_channel_wait_for_finish_blocking(dma_chan);

    // capture finished
    adc_run(false);
    adc_fifo_drain();
}

inline bool AnalogSensorBase::_is_channel_already_used(uint8_t adc_channel) // static
{
    //   adc_channel is in [0,3]       adc_channel->mask
    return (adc_channel < 4)    &&    ((1 << adc_channel) & _adc_used_channels_mask);
}

inline void AnalogSensorBase::_claim_adc_channel(uint8_t adc_channel) // static
{
    if (adc_channel > 3) return;
    _adc_used_channels_mask |= (1 << adc_channel);
}

void AnalogSensorBase::read_raw_avg_blocking_online(size_t n, ANALOG_SENSOR_RESOLUTION *mean, ANALOG_SENSOR_RESOLUTION *stdev)
{
    uint16_t raws[n] = {0};

    #if _ANALOG_SENSOR_USE_DMA
    if (_selected_channel != _adc_channel)
    {
        adc_select_input(_adc_channel);
        _selected_channel = _adc_channel;
    }
    read_adc_dma(raws, n);
    #else
    for (size_t i = 0; i < n; i++)
        raws[i] = read_raw_single_blocking();
    #endif

    calc_mean_stdev_online<uint16_t, ANALOG_SENSOR_RESOLUTION>(raws, n, mean, stdev);
}

void AnalogSensorBase::read_raw_avg_blocking_offline(size_t n, ANALOG_SENSOR_RESOLUTION *mean, ANALOG_SENSOR_RESOLUTION *stdev)
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

AnalogSensorBase::AnalogSensorBase(uint pin)
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

void AnalogSensorBase::begin()
{
    if (!_adc_init_flag)
    {
        adc_init();
        _adc_init_flag = true;
    }

    adc_gpio_init(_pin);
}

uint16_t AnalogSensorBase::read_raw_single_blocking()
{
    if (_selected_channel != _adc_channel)
    {
        adc_select_input(_adc_channel);
        _selected_channel = _adc_channel;
    }
    return adc_read();
}

struct AnalogSensorAvgResult AnalogSensorBase::read_raw_avg_blocking(size_t n)
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

    #ifdef D_WARN
    if (!res.count)
        D_WARN("Analog sensor avg reading returned with count = 0");
    #endif

    return res;
}






AnalogSensorLinear::AnalogSensorLinear(uint pin, ANALOG_SENSOR_RESOLUTION slope, ANALOG_SENSOR_RESOLUTION slope_error, ANALOG_SENSOR_RESOLUTION offset, ANALOG_SENSOR_RESOLUTION offset_error)
    : IAnalogSensor(pin), _slope(slope), _offset(offset), _slope_error(slope_error), _offset_error(offset_error)
{}

struct AnalogSensorAvgResult AnalogSensorLinear::read_avg_blocking(size_t n)
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






AnalogSensorExponential::AnalogSensorExponential(uint pin, ANALOG_SENSOR_RESOLUTION a, ANALOG_SENSOR_RESOLUTION b, ANALOG_SENSOR_RESOLUTION c, ANALOG_SENSOR_RESOLUTION d,
                      ANALOG_SENSOR_RESOLUTION a_error, ANALOG_SENSOR_RESOLUTION b_error, ANALOG_SENSOR_RESOLUTION c_error, ANALOG_SENSOR_RESOLUTION d_error)
    : IAnalogSensor(pin), _a(a), _b(b), _c(c), _d(d),
        _a_error(a_error), _b_error(b_error), _c_error(c_error), _d_error(d_error)
{}

struct AnalogSensorAvgResult AnalogSensorExponential::read_avg_blocking(size_t n)
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





AnalogSensorLogarithmic::AnalogSensorLogarithmic(uint pin, ANALOG_SENSOR_RESOLUTION a, ANALOG_SENSOR_RESOLUTION b, ANALOG_SENSOR_RESOLUTION c, ANALOG_SENSOR_RESOLUTION d,
                    ANALOG_SENSOR_RESOLUTION a_error, ANALOG_SENSOR_RESOLUTION b_error, ANALOG_SENSOR_RESOLUTION c_error, ANALOG_SENSOR_RESOLUTION d_error)
    : IAnalogSensor(pin), _a(a), _b(b), _c(c), _d(d),
        _a_error(a_error), _b_error(b_error), _c_error(c_error), _d_error(d_error)
{}

struct AnalogSensorAvgResult AnalogSensorLogarithmic::read_avg_blocking(size_t n)
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