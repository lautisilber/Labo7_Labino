#ifndef HX711_H
#define HX711_H

#include "hx711_driver.h"
#include "user_flash_class.hpp"
#include <pico/stdlib.h>
#include <assert.h>


#define HX711_MUX_N_PINS                                          4
#define HX711_MUX_MAX_MODULES                                     1 << HX711_MUX_N_PINS // same as 2**(HX711_MUX_N_PINS)
#define HX711_MUX_MULTIPLEXER_TRANSITION_TIME_US                  10 // I think the sn74hc251 and the sn74hc259 have a transition time of 50 ns

#define HX711_MUX_MAX_ITERATIONS_BEFORE_USING_OFFLINE_MEAN_STDEV  256

#define HX711_DEFAULT_FLASH_USER                                  0

struct HX711MuxCalibration
{
    // sizeof(this) = 20
    float offset, slope, offset_error, slope_error;
    bool offset_calib_state=false, slope_calib_state=false;
};

#define HX711_MUX_USER_FLASH_LENGTH sizeof(struct HX711MuxCalibration) * HX711_MUX_MAX_MODULES
static_assert(
    HX711_MUX_USER_FLASH_LENGTH <= FLASH_USER_SAVE_BYTES_SIZE,
    "Not enough flash user space to save all calibrations!"
);

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
    size_t n_missed_reads;
    size_t n_filtered_reads;
};

struct HX711ResponseAvgCalib //: HX711ResponseAvg
{
    bool success;
    float mean;
    float stdev;
    size_t count;
    size_t n_missed_reads;
    size_t n_filtered_reads;
};

class HX711Mux : UserFlashBase
{
private:
    struct HX711 _hx711;

    uint _mux_pins[HX711_MUX_N_PINS];
    struct HX711MuxCalibration _calibrations[HX711_MUX_MAX_MODULES];
    uint32_t _timeout_ms;
    bool _connected_addresses[HX711_MUX_N_PINS] = {false};
    uint8_t _current_address;

private:
    inline bool is_address_available(uint8_t address);
    bool is_address_connected(uint8_t address);
    bool set_address(uint8_t address);
    bool read_raw_single_dont_change_address(int32_t *raw);
    void read_avg_single_online_dont_change_address(struct HX711ResponseAvg *res, size_t n);
    void read_avg_single_offline_dont_change_address(struct HX711ResponseAvg *res, size_t n);

public:
    HX711Mux(uint dout_pin, uint pd_sck_pin, uint mux_pins[HX711_MUX_N_PINS], enum HX711Gain gain, uint32_t timeout_ms, uint32_t flash_user_index=HX711_DEFAULT_FLASH_USER);
    HX711Mux(uint dout_pin, uint pd_sck_pin, uint mux_pins[HX711_MUX_N_PINS], enum HX711Gain gain, uint32_t timeout_ms, bool default_addresses_states[HX711_MUX_MAX_MODULES], uint32_t flash_user_index=HX711_DEFAULT_FLASH_USER);

    bool set_address_state(uint8_t address, bool connected);
    bool set_addresses_state(uint8_t *addresses, uint8_t n_addresses, bool connected);

    void all_power_down();
    void all_power_up();

    void begin(bool begin_power_down);

    struct HX711ResponseRaw read_raw_single(uint8_t address);
    struct HX711ResponseAvg read_avg_single(uint8_t address, size_t n);
    struct HX711ResponseAvgCalib read_calib_single(uint8_t address, size_t n);

    bool calibrate_offset_single(uint8_t address, size_t n);
    bool calibrate_slope_single(uint8_t address, size_t n, float weight, float weight_error);

    bool save_calibrations_to_flash();
    bool load_calibrations_from_flash();
};


#endif /* HX711_H */