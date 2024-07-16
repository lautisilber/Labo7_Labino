#ifndef HX711_H
#define HX711_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hx711_driver.h"
#include "timer_interrupts_helper.h"
#include "utils.hpp"
#include <string.h>


#define HX711_MUX_N_PINS                                          4
#define HX711_MUX_MAX_MODULES                                     1 << HX711_MUX_N_PINS // same as 2**(HX711_MUX_N_PINS)
#define HX711_MUX_MULTIPLEXER_TRANSITION_TIME_US                  10 // I think the sn74hc251 and the sn74hc259 have a transition time of 50 ns

#define HX711_MUX_MAX_ITERATIONS_BEFORE_USING_OFFLINE_MEAN_STDEV  256

struct HX711MuxCalibration
{
    float offset, slope;  
};

struct HX711ResponseRaw
{
    bool success;
    int32_t result;
};

struct HX711ResponseAvg
{
    bool success;
    float mean;
    float stdev;
    size_t count;
};

class HX711Mux
{
private:
    uint _mux_pins[HX711_MUX_N_PINS];
    HX711MuxCalibration _calibrations[HX711_MUX_MAX_MODULES];
    uint32_t _timeout_ms;
    uint8_t _current_address;

    struct HX711 _hx711;

private:
    inline bool is_address_available(uint8_t address)
    {
        return address < HX711_MUX_MAX_MODULES;
    }

    bool set_address(uint8_t address)
    {
        if (!is_address_available(address)) return false;

        bool binary[get_bit_size<uint8_t>()];
        dec_2_bin<uint8_t>(address, binary);

        uint32_t mask = 0;
        for (uint8_t i = 0; i < HX711_MUX_N_PINS; i++)
        {
            if (binary[i])
                mask |= (1 << _mux_pins[i]);
        }

        gpio_put_masked(mask, true);
    }

    bool read_raw_single_dont_change_address(int32_t *raw)
    {
        timer_disable_irq();
        return hx711_read(&_hx711, raw, _timeout_ms);
        timer_enable_irq();
    }

    void read_avg_single_online_dont_change_address(struct HX711ResponseAvg *res, size_t n)
    {
        // int32_t raws[n] = {0};
        // bool raw_successes[n] = {false};

        // for (size_t i = 0; i < n; ++i)
        // {
        //     bool s = read_raw_single_dont_change_address(&raws[i]);
        //     if (s)
        //     {
        //         raw_successes[i] = true;
        //     }
        // }

        // size_t n_successes = remove_by_mask(raws, raw_successes, n);

        int32_t raw = 0;
        int32_t raws[n] = {0};
        size_t n_successes = 0;

        for (size_t i = 0; i < n; ++i)
        {
            if (read_raw_single_dont_change_address(&raw))
            {
                raws[n_successes++] = raw;
            }
        }

        calc_mean_stdev_online<int32_t, float>(raws, n_successes, &res->mean, &res->stdev);
        res->count = n_successes;
        res->success = true;
    }

    void read_avg_single_offline_dont_change_address(struct HX711ResponseAvg *res, size_t n)
    {
        struct WelfordAggregate<float> welf_agg = {.count=0, .mean=0.0f, .m2=0.0f};
        int32_t raw;

        for (size_t i = 0; i < n; ++i)
        {
            if (read_raw_single_dont_change_address(&raw))
            {
                calc_mean_stdev_welford<int32_t, float>(&welf_agg, raw);
            }
        }

        res->success = calc_mean_stdev_welford_finish<int32_t, float>(&welf_agg, &res->mean, &res->stdev);
        res->count = welf_agg.count;
    }

public:
    HX711Mux(uint dout_pin, uint pd_sck_pin, uint mux_pins[HX711_MUX_N_PINS], enum HX711Gain gain, uint32_t timeout_ms)
        : _hx711{.dout=dout_pin, .pd_sck=pd_sck_pin, .gain=gain}, _timeout_ms(timeout_ms)
    {
        memcpy(_mux_pins, mux_pins, HX711_MUX_N_PINS*sizeof(uint));
    }

    void all_power_down()
    {
        hx711_power_down(&_hx711);
        for (uint8_t i = HX711_MUX_MAX_MODULES; i >= 0; i--)
        {
            set_address(i);
            sleep_us(HX711_MUX_MULTIPLEXER_TRANSITION_TIME_US);
        }
    }

    void all_power_up()
    {
        hx711_power_up(&_hx711);
        for (uint8_t i = HX711_MUX_MAX_MODULES; i >= 0; i--)
        {
            set_address(i);
            sleep_us(HX711_MUX_MULTIPLEXER_TRANSITION_TIME_US);
        }
    }

    void begin(bool begin_power_down)
    {
        // mask representing the only relevant in pin: the dout pin
        uint32_t in_mask  = (1 << _hx711.dout);
        // create mask for all pins that are output = pd_sck and all mux pins
        uint32_t out_mask = (1 << _hx711.pd_sck);
        for (uint8_t i = 0; i < HX711_MUX_N_PINS; i++)
            out_mask |= (1 << _mux_pins[i]);

        // initialize pins accourdingly
        gpio_init_mask(out_mask || in_mask);
        gpio_set_dir_out_masked(out_mask);
        gpio_set_dir_in_masked(in_mask);

        // set the signal to be the corresponding begin state
        if (begin_power_down)
            all_power_down();
        else
            all_power_up();
    }

    struct HX711ResponseRaw read_raw_single(uint8_t address)
    {
        struct HX711ResponseRaw res = {.success=false, .result=0};

        if (!set_address(address)) return res;
        res.success = read_raw_single_dont_change_address(&res.result);
        return res;
    }

    struct HX711ResponseAvg read_avg_single(uint8_t address, size_t n)
    {
        struct HX711ResponseAvg res = {.success=false, .mean=0.0, .stdev=0.0, .count=0};
        
        if (!set_address(address))
            return res;

        if (n > HX711_MUX_MAX_ITERATIONS_BEFORE_USING_OFFLINE_MEAN_STDEV)
        {
            read_avg_single_online_dont_change_address(&res, n);
        }
        else
        {
            read_avg_single_offline_dont_change_address(&res, n);
        }

        return res;
    }
};


#endif /* HX711_H */