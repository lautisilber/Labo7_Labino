#ifndef HX711_H
#define HX711_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hx711_driver.h"
#include "timer_interrupts_helper.h"
#include <string.h>


#define HX711_MUX_N_PINS                          4
#define HX711_MUX_MAX_MODULES                     1 << HX711_MUX_N_PINS // same as 2**(HX711_MUX_N_PINS)
#define HX711_MUX_MULTIPLEXER_TRANSITION_TIME_US  10 // I think the sn74hc251 and the sn74hc259 have a transition time of 50 ns

struct HX711MuxCalibration
{
    float offset, slope;  
};

class HX711Mux
{
private:
    uint _mux_pins[HX711_MUX_N_PINS];
    HX711MuxCalibration _calibrations[HX711_MUX_MAX_MODULES];
    uint32_t _timeout_ms;

    struct HX711 _hx711;

private:
    inline bool is_address_available(uint8_t address)
    {
        return address < HX711_MUX_MAX_MODULES;
    }

    bool set_address(uint8_t address)
    {
        // https://www.scaler.com/topics/decimal-to-binary-in-c
        if (!is_address_available(address)) return false;

        bool binary[HX711_MUX_N_PINS] = {0};

        for (uint8_t i = 0; i < HX711_MUX_N_PINS && address > 0; i++)
        {
            binary[i] = address & 1;
            address = address >> 1;
        }

        uint32_t mask = 0;
        for (uint8_t i = 0; i < HX711_MUX_N_PINS; i++)
        {
            if (binary[i])
                mask |= (1 << _mux_pins[i]);
        }

        gpio_put_masked(mask, true);
    }

public:
    HX711Mux(uint dout_pin, uint pd_sck_pin, uint mux_pins[HX711_MUX_N_PINS], enum HX711Gain gain, uint32_t timeout_ms)
        : _hx711{.dout=dout_pin, .pd_sck=pd_sck_pin, .gain=gain}, _timeout_ms(timeout_ms)
    {
        memcpy(_mux_pins, mux_pins, HX711_MUX_N_PINS*sizeof(uint));
    }

    void all_power_off()
    {
        hx711_power_down(&_hx711);
        for (uint8_t i = HX711_MUX_MAX_MODULES; i >= 0; i--)
        {
            set_address(i);
            sleep_us(HX711_MUX_MULTIPLEXER_TRANSITION_TIME_US);
        }
    }

    void all_power_on()
    {
        hx711_power_up(&_hx711);
        for (uint8_t i = HX711_MUX_MAX_MODULES; i >= 0; i--)
        {
            set_address(i);
            sleep_us(HX711_MUX_MULTIPLEXER_TRANSITION_TIME_US);
        }
    }

    void begin()
    {
        uint32_t in_mask  = (1 << _hx711.dout);
        uint32_t out_mask = (1 << _hx711.pd_sck);
        for (uint8_t i = 0; i < HX711_MUX_N_PINS; i++)
        {
            out_mask |= (1 << _mux_pins[i]);
        }

        gpio_init_mask(out_mask || in_mask);
        gpio_set_dir_out_masked(out_mask);
        gpio_set_dir_in_masked(in_mask);

        all_power_off();
    }

    bool read_raw_single(int32_t *raw)
    {
        timer_disable_irq();
        return hx711_read(&_hx711, raw, _timeout_ms);
        timer_enable_irq();
    }
};


#endif /* HX711_H */