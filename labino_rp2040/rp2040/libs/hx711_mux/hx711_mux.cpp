#include "hx711_mux.hpp"

#include <hardware/gpio.h>
#include <hardware/sync.h>
#include <string.h>
#include <math.h>

#include "utils.hpp"


inline bool HX711Mux::is_address_available(uint8_t address)
{
    return address < HX711_MUX_MAX_MODULES;
}

bool HX711Mux::is_address_connected(uint8_t address)
{
    if (!is_address_available(address)) return false;
    return _connected_addresses[address];
}

bool HX711Mux::set_address(uint8_t address)
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

    return true;
}

bool HX711Mux::read_raw_single_dont_change_address(int32_t *raw)
{
    uint32_t interrupts = save_and_disable_interrupts();
    return hx711_read(&_hx711, raw, _timeout_ms);
    restore_interrupts(interrupts);
}

void HX711Mux::read_avg_single_online_dont_change_address(struct HX711ResponseAvg *res, size_t n)
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

    // this is only possible in the online calculation of mean and stdev, because we need all
    // data points to make the irq filtering
    size_t new_len = interquartile_range_filter<int32_t, float>(raws, n_successes, 1.5f);

    calc_mean_stdev_online<int32_t, float>(raws, new_len, &res->mean, &res->stdev);
    res->count = new_len;
    res->n_filtered_reads = n_successes - new_len;
    res->n_missed_reads = n - n_successes;
    res->success = true;
}

void HX711Mux::read_avg_single_offline_dont_change_address(struct HX711ResponseAvg *res, size_t n)
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
    res->n_filtered_reads = 0; // can't do irq filtering offline
    res->n_missed_reads = n - welf_agg.count;
}

HX711Mux::HX711Mux(uint dout_pin, uint pd_sck_pin, uint mux_pins[HX711_MUX_N_PINS], enum HX711Gain gain, uint32_t timeout_ms, uint32_t flash_user_index)
    : _hx711{.dout=dout_pin, .pd_sck=pd_sck_pin, .gain=gain}, _timeout_ms(timeout_ms), UserFlashBase(HX711_MUX_USER_FLASH_LENGTH)
{
    memcpy(_mux_pins, mux_pins, HX711_MUX_N_PINS*sizeof(uint));
}

HX711Mux::HX711Mux(uint dout_pin, uint pd_sck_pin, uint mux_pins[HX711_MUX_N_PINS], enum HX711Gain gain, uint32_t timeout_ms, bool default_addresses_states[HX711_MUX_MAX_MODULES], uint32_t flash_user_index)
    : HX711Mux(dout_pin, pd_sck_pin, mux_pins, gain, timeout_ms)
{
    memcpy(_connected_addresses, default_addresses_states, HX711_MUX_MAX_MODULES*sizeof(bool));
}

bool HX711Mux::set_address_state(uint8_t address, bool connected)
{
    if (!is_address_available(address)) return false;
    _connected_addresses[address] = connected;
    return true;
}

bool HX711Mux::set_addresses_state(uint8_t *addresses, uint8_t n_addresses, bool connected)
{
    // addresses is an array containing the hx711 connector addresses whose state should be changed to <connected>
    for (uint8_t i = 0; i < n_addresses; i++)
    {
        // one or more addresses are not valid
        if (!is_address_available(addresses[i])) return false;
    }

    for (uint8_t i = 0; i < n_addresses; i++)
    {
        _connected_addresses[addresses[i]] = connected;
    }

    return true;
}

void HX711Mux::all_power_down()
{
    hx711_power_down(&_hx711);
    for (uint8_t i = HX711_MUX_MAX_MODULES; i >= 0; i--)
    {
        if (!_connected_addresses[i]) continue; // skip non connected addresses
        set_address(i);
        sleep_us(HX711_MUX_MULTIPLEXER_TRANSITION_TIME_US);
    }
}

void HX711Mux::all_power_up()
{
    hx711_power_up(&_hx711);
    for (uint8_t i = HX711_MUX_MAX_MODULES; i >= 0; i--)
    {
        if (!_connected_addresses[i]) continue; // skip non connected addresses
        set_address(i);
        sleep_us(HX711_MUX_MULTIPLEXER_TRANSITION_TIME_US);
    }
}

void HX711Mux::begin(bool begin_power_down)
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

struct HX711ResponseRaw HX711Mux::read_raw_single(uint8_t address)
{
    struct HX711ResponseRaw res = {.success=false, .result=0};

    if (!set_address(address)) return res;
    res.success = read_raw_single_dont_change_address(&res.result);
    return res;
}

struct HX711ResponseAvg HX711Mux::read_avg_single(uint8_t address, size_t n)
{
    struct HX711ResponseAvg res = {.success=false, .mean=0.0, .stdev=0.0, .count=0};
    
    if (!set_address(address))
        return res;

    if (n == 0) return res;
    else if (n == 1)
    {
        int32_t raw;
        if (read_raw_single_dont_change_address(&raw))
        {
            res.count = 1;
            res.success = true;
            res.mean = (float)raw;
            res.stdev = 0.0f;
        }
    }
    else if (n <= HX711_MUX_MAX_ITERATIONS_BEFORE_USING_OFFLINE_MEAN_STDEV)
    {
        read_avg_single_online_dont_change_address(&res, n);
    }
    else
    {
        read_avg_single_offline_dont_change_address(&res, n);
    }

    return res;
}

struct HX711ResponseAvgCalib HX711Mux::read_calib_single(uint8_t address, size_t n)
{
    HX711ResponseAvgCalib res_calib = { .success=false };
    if (!_calibrations[address].offset_calib_state || !_calibrations[address].slope_calib_state) return res_calib;

    struct HX711ResponseAvg res_avg = read_avg_single(address, n);
    res_calib.count = res_avg.count;
    res_calib.n_missed_reads = res_avg.n_missed_reads;
    res_calib.n_filtered_reads = res_avg.n_filtered_reads;
    if (!res_avg.success) return res_calib;

    linear_error_propagation<float>(&res_calib.mean, &res_calib.stdev,
                                    res_avg.mean, res_avg.stdev,
                                    _calibrations[address].slope, _calibrations[address].slope_error,
                                    _calibrations[address].offset, _calibrations[address].offset_error);

    return res_calib;
}

bool HX711Mux::calibrate_offset_single(uint8_t address, size_t n)
{
    if (!is_address_connected(n)) return false;

    const struct HX711ResponseAvg res = read_avg_single(address, n);
    if (!res.success) return false;

    _calibrations[address].offset = res.mean;
    _calibrations[address].offset_error = res.stdev;

    return true;
}

bool HX711Mux::calibrate_slope_single(uint8_t address, size_t n, float weight, float weight_error)
{
    // TODO: test this

    if (!is_address_connected(n)) return false;
    if (!_calibrations[address].offset_calib_state) return false; // no offset calibration has been done

    const struct HX711ResponseAvg res = read_avg_single(address, n);
    if (!res.success) return false;

    // https://en.wikipedia.org/wiki/Propagation_of_uncertainty
    // slope = dy/dx
    // r = raw value, o = offset, s = slope, v = calibrated value
    // I want to do v = r * s + o
    // which yields (v - o) / r = s
    // so, havinc calculated the offset, I can get the value from the user and measure the raw value
    //
    // doing that, we get
    //
    // s = (v - o) / r
    // s_error = sqrt( (o_err^2 + v_err^2)/r^2 + (v - o)^2 * (r_err / r^2)^2 )

    const float o      = _calibrations[address].offset;
    const float o_err  = _calibrations[address].offset_error;
    const float o_err2 = o_err * o_err;
    const float r      = res.mean;
    const float r2     = r * r;
    const float r_err  = res.stdev;
    const float v      = weight;
    const float v_err  = weight_error;
    const float v_err2 = v_err * v_err;

    const float v_o    = v - o;
    const float v_o2   = v_o * v_o;

    const float r_err_over_r2   = r != 0.0f ? r_err / r2 : 0.0f;
    const float r_err_over_r2_2 = r_err_over_r2 * r_err_over_r2;

    _calibrations[address].slope = (v - o) / r;
    _calibrations[address].slope_error = sqrt( (o_err2 + v_err2) / r2 + v_o2 * r_err_over_r2_2 );
    _calibrations[address].slope_calib_state = true;

    return true;
}

void HX711Mux::save_calibrations_to_flash()
{
    base_flash_save(&_calibrations);
}

void HX711Mux::load_calibrations_from_flash()
{
    base_flash_load(&_calibrations);
}