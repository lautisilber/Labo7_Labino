#include <stdio.h>
#include <string.h>

#include "bme280_class.hpp"
#include "i2c_helper.h"

/// DEFINES AND DECLARATIONS ///

static void print_error(int8_t err_code);

#define _BME280_CLASS_PRINT_ERROR(err_code) print_error(err_code)

// Recommended mode of operation for weather monitoring
// Testing is needed, but I'd add 2x or 4x oversampling
// to temperature and humidity
static const struct bme280_settings default_settings = {
    .osr_p = BME280_OVERSAMPLING_1X,
    .osr_t = BME280_OVERSAMPLING_1X,
    .osr_h = BME280_OVERSAMPLING_1X,
    .filter = BME280_FILTER_COEFF_OFF,
};

// i2c callbacks
// typedef BME280_INTF_RET_TYPE (*bme280_read_fptr_t)(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
static BME280_INTF_RET_TYPE bme280_read_i2c(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    // int i2c_read_blocking_until (i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop, absolute_time_t until)
    const bool nonstop = false;               // if false, release bus
    const uint timeout_us = 10 * 1000 * 1000; // 10 s
    int rslt = i2c_read_timeout_us(BME280_STOMASENSE_I2C_INTERFACE, reg_addr, reg_data, len, nonstop, timeout_us);
    if (rslt == PICO_ERROR_TIMEOUT)
        return -1;
    else if (rslt == PICO_ERROR_GENERIC)
        return -2;
    else if (rslt != len)
        return -3;
    return BME280_INTF_RET_SUCCESS;
}

// typedef BME280_INTF_RET_TYPE (*bme280_write_fptr_t)(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
static BME280_INTF_RET_TYPE bme280_write_i2c(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    // int i2c_write_blocking_until (i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop, absolute_time_t until)
    const bool nonstop = false;               // if false, release bus
    const uint timeout_us = 10 * 1000 * 1000; // 10 s
    int rslt = i2c_write_timeout_us(BME280_STOMASENSE_I2C_INTERFACE, reg_addr, reg_data, len, nonstop, timeout_us);
    if (rslt == PICO_ERROR_TIMEOUT)
        return -1;
    else if (rslt == PICO_ERROR_GENERIC)
        return -2;
    else if (rslt != len)
        return -3;
    return BME280_INTF_RET_SUCCESS;
}

// typedef void (*bme280_delay_us_fptr_t)(uint32_t period, void *intf_ptr);
static void bme280_delay_us_i2c(uint32_t period_us, void *intf_ptr)
{
    sleep_us(period_us);
}

///////////////////////////////

int64_t BME280_I2C::_alarm_callback(alarm_id_t id, void *user_data)
{
    // TODO: check that this variable is being changed atomicaly
    ((BME280_I2C *)user_data)->_can_measure_flag = true;
    return 0; // do not reschedule alarm automaticaly
}

bool BME280_I2C::_schedule_measurement_alarm_flag(uint32_t in_ms)
{
    if (_can_measure_flag < 0)
        return false;
    alarm_id_t id = add_alarm_in_ms(in_ms, _alarm_callback, this, true);
    return id >= 0;
}

bool BME280_I2C::calculate_measurement_delay()
{
    if (_init_flag)
    {
        return false;
    }

    uint32_t max_delay_ms_temp;
    int8_t rslt = bme280_cal_meas_delay(&max_delay_ms_temp, &_settings);
    _BME280_CLASS_PRINT_ERROR(rslt);
    bool res = rslt == BME280_OK;

    if (res)
        _max_delay_ms = max_delay_ms_temp;

    return res;
}

BME280_I2C::BME280_I2C(uint sda_pin, uint scl_pin, uint baudrate,
                       const struct bme280_settings *settings)
    : _sda_pin(sda_pin), _scl_pin(scl_pin), _baudrate(baudrate)
{
    const struct bme280_settings *settings_ptr = (settings ? settings : &default_settings);
    memcpy(&_settings, settings_ptr, sizeof(struct bme280_settings));
}

bool BME280_I2C::begin(bool dont_i2c_init, bool pullups)
{
    // init i2c hardware
    if (dont_i2c_init)
    {
        i2c_init_helper_only_pins(_sda_pin, _scl_pin, pullups);
    }
    else
    {
        i2c_init_helper_full(BME280_STOMASENSE_I2C_INTERFACE, _baudrate, _sda_pin, _scl_pin, pullups);
    }

    // init bme280_dev struct
    _dev.intf = BME280_I2C_INTF;
    _dev.read = bme280_read_i2c;
    _dev.write = bme280_write_i2c;
    _dev.delay_us = bme280_delay_us_i2c;

    // init bme280_dev hardware
    int8_t init_res = bme280_init(&_dev);
    _BME280_CLASS_PRINT_ERROR(init_res);

    // init bme280_dev hardware settings & measurement
    bool settings_res = set_sensor_settings(&_settings);

    _init_flag = init_res == BME280_OK && settings_res;
    return _init_flag;
}

bool BME280_I2C::set_sensor_settings(const struct bme280_settings *settings)
{
    int8_t settings_res = bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &_settings, &_dev);
    _BME280_CLASS_PRINT_ERROR(settings_res);

    bool res = settings_res == BME280_OK;
    if (!res)
        return false;

    return calculate_measurement_delay();
}

bool BME280_I2C::get_sensor_settings(struct bme280_settings *settings)
{
    if (_init_flag)
    {
        return false;
    }

    int8_t res = bme280_get_sensor_settings(settings, &_dev);
    _BME280_CLASS_PRINT_ERROR(res);
    return res == BME280_OK;
}

bool BME280_I2C::set_sensor_mode(uint8_t sensor_mode)
{
    if (_init_flag)
    {
        return false;
    }

    int8_t res = bme280_set_sensor_mode(sensor_mode, &_dev);
    _BME280_CLASS_PRINT_ERROR(res);
    return res == BME280_OK;
}

bool BME280_I2C::soft_reset()
{
    if (_init_flag)
    {
        return false;
    }

    int8_t res = bme280_soft_reset(&_dev);
    _BME280_CLASS_PRINT_ERROR(res);
    return res == BME280_OK;
}

bool BME280_I2C::get_sensor_data(const bme280_data *data)
{
    if (!_can_measure_flag)
    {
        data = nullptr;
        return false;
    }

    if (_init_flag)
    {
        return false;
    }

    int8_t res = bme280_get_sensor_data(BME280_ALL, &_data, &_dev);

    _BME280_CLASS_PRINT_ERROR(res);

    _can_measure_flag = false;
    bool alarm_scheduled = _schedule_measurement_alarm_flag(
        (_max_delay_ms > 0 ? _max_delay_ms : BME280_CLASS_MIN_MEASUREMENT_DELAY_MS) + 10); // add 10 milliseconds, just in case and clamp minimum if delay is 0
    if (!alarm_scheduled)
        _can_measure_flag = true; // a fallback to avoid softlocking

    return get_last_sensor_data();
}

static void print_error(int8_t err_code)
{
    switch (err_code)
    {
    case BME280_OK:
        // printf("BME280 OK\n");
        break;
    case BME280_E_NULL_PTR:
        printf("BME280 ERROR: NULL bme280 dev pointer\n");
        break;
    case BME280_E_COMM_FAIL:
        printf("BME280 ERROR: communication failure\n");
        break;
    case BME280_E_INVALID_LEN:
        printf("BME280 ERROR: invalid length\n");
        break;
    case BME280_E_DEV_NOT_FOUND:
        printf("BME280 ERROR: device not found\n");
        break;
    case BME280_E_SLEEP_MODE_FAIL:
        printf("BME280 ERROR: sleep mode fail\n");
        break;
    case BME280_E_NVM_COPY_FAILED:
        printf("BME280 ERROR: NVM copy failed\n");
        break;
    case BME280_W_INVALID_OSR_MACRO:
        printf("BME280 WARNING: invalid OSR macro\n");
        break;
    case BME280_I2C_BAD_REFERENCE_SET:
        printf("BME280 I2C ERROR: bad i2c device reference\n");
        break;
    case BME280_I2C_COULDNT_SET_BAUDRATE:
        printf("BME280 I2C ERROR: couldn't set i2c baudrate\n");
        break;
    case BME280_CLASS_NOT_INIT:
        printf("BME280 CLASS ERROR: bme280 device not initialised\n");
        break;
    case BME280_CLASS_NOT_ENOUGH_TIME_BETWEEN_READS:
        printf("BME280 CLASS ERROR: not enough time between readsd\n");
        break;
    default:
        printf("BME280: unknown error");
        break;
    }
}
