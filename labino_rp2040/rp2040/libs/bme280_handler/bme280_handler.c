#include "bme280_handler.h"
#include "bme280_defs.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

static const struct bme280_settings default_settings = {
    .osr_p = BME280_OVERSAMPLING_16X,
    .osr_t = BME280_OVERSAMPLING_16X,
    .osr_h = BME280_OVERSAMPLING_16X,
    .filter = BME280_FILTER_COEFF_OFF,
};

static struct bme280_dev dev = {};
static struct bme280_data dev_data = {};
static struct bme280_settings dev_settings = {};
static int32_t cal_meas_delay = -1;

static void print_error(int8_t err_code)
{
    switch (err_code) {
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
        case BME280_HANDLER_NOT_INIT:
            printf("BME280 HANDLER ERROR: bme280 device not initialised\n");
            break;
        case BME280_HANDLER_NOT_ENOUGH_TIME_BETWEEN_READS:
            printf("BME280 HANDLER ERROR: not enough time between readsd\n");
            break;
        default:
            printf("BME280: unknown error");
            break;
    }
}

static inline bool is_dev_initialised()
{
    return !((dev.read == NULL) || (dev.write == NULL) || (dev.delay_us == NULL) || cal_meas_delay < 0);
}

#define _BME280_HANDLER_PRINT_ERROR(err_code) print_error(err_code)

#define _BME280_HANDLER_CHECK_INIT() \
do { \
    if (!is_dev_initialised()) { \
        _BME280_HANDLER_PRINT_ERROR(BME280_HANDLER_NOT_INIT); \
        return false; \
    } \
} while (0)

static bool bme280_handler_cal_meas_delay_internal()
{
    _BME280_HANDLER_CHECK_INIT();

    uint32_t cal_meas_delay_temp;
    int8_t rslt = bme280_cal_meas_delay(&cal_meas_delay_temp, &dev_settings);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    bool res = rslt == BME280_OK;

    if (res)
        cal_meas_delay = (int32_t)cal_meas_delay_temp;

    return res;
}

// due to the sensor having to take time between measurements, we have to wait between polls.
// When we measure, we start an alarm that will wait the maximum required time between
// measurements give the sensor settings.
static bool can_measure_again = true;
static int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    // TODO: check that this variable is being changed atomicaly
    can_measure_again = true;
    return 0; // do not reschedule alarm automaticaly
}
static bool schedule_measurement_alarm_flag(uint32_t in_ms)
{
    if (cal_meas_delay < 0) return false;
    alarm_id_t id = add_alarm_in_ms(in_ms, alarm_callback, NULL, true);
    return id >= 0;
}

bool bme280_handler_init(i2c_inst_t *i2c_dev,
                        bool dont_i2c_init,
                        uint baudrate,
                        const struct bme280_settings *settings)
{
    int8_t rslt;

    if(dont_i2c_init)
        rslt = bme280_init_i2c(&dev, i2c_dev);
    else
        rslt = bme280_init_force_i2c(&dev, i2c_dev, baudrate);
    _BME280_HANDLER_PRINT_ERROR(rslt);

    bool r1 = bme280_handler_set_sensor_settings(BME280_SEL_ALL_SETTINGS, settings);
    bool r2 = bme280_handler_cal_meas_delay_internal();

    return r1 && r2;
}

bool bme280_handler_set_sensor_settings(uint8_t desired_settings,
                                  const struct bme280_settings *settings)
{
    _BME280_HANDLER_CHECK_INIT();

    const struct bme280_settings *settings_ptr = settings == NULL ? &default_settings : settings;

    int8_t rslt = bme280_set_sensor_settings(desired_settings, settings_ptr, &dev);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    bool r1 = rslt == BME280_OK;
    if (r1)
    {
        memcpy(&dev_settings, settings_ptr, sizeof(struct bme280_settings));
    }

    bool r2 = bme280_handler_cal_meas_delay_internal();

    return r1 && r2;
}

bool bme280_handler_get_sensor_settings(struct bme280_settings *settings)
{
    _BME280_HANDLER_CHECK_INIT();

    int8_t rslt = bme280_get_sensor_settings(settings, &dev);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    return rslt == BME280_OK;
}

bool bme280_handler_set_sensor_mode(uint8_t sensor_mode)
{
    _BME280_HANDLER_CHECK_INIT();

    int8_t rslt = bme280_set_sensor_mode(sensor_mode, &dev);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    return rslt == BME280_OK;
}

bool bme280_handler_get_sensor_mode(uint8_t *sensor_mode)
{
    _BME280_HANDLER_CHECK_INIT();

    int8_t rslt = bme280_get_sensor_mode(sensor_mode, &dev);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    return rslt == BME280_OK;
}

bool bme280_handler_soft_reset()
{
    _BME280_HANDLER_CHECK_INIT();

    int8_t rslt = bme280_soft_reset(&dev);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    return rslt == BME280_OK;
}

const struct bme280_data* bme280_handler_get_sensor_data()
{
    _BME280_HANDLER_CHECK_INIT();

    if (!can_measure_again)
    {
        _BME280_HANDLER_PRINT_ERROR(BME280_HANDLER_NOT_ENOUGH_TIME_BETWEEN_READS);
        return NULL;
    }

    int8_t rslt = bme280_get_sensor_data(BME280_ALL, &dev_data, &dev);

    can_measure_again = false;
    bool alarm_scheduled = schedule_measurement_alarm_flag(cal_meas_delay + 10);
    if (!alarm_scheduled) can_measure_again = true; // a fallback to avoid softlocking

    _BME280_HANDLER_PRINT_ERROR(rslt);
    if (rslt == BME280_OK)
        return (const struct bme280_data*)&dev_data;
    else
        return NULL;
}

bool bme280_handler_cal_meas_delay(uint32_t *max_delay_ms, bool force_update)
{
    if (force_update)
    {
        bool res = bme280_handler_cal_meas_delay_internal();
        if (res)
            *max_delay_ms = cal_meas_delay;
        return res;
    }
    else
    {
        *max_delay_ms = cal_meas_delay;
        return true;
    }
}

bool bme280_handler_can_measure()
{
    return can_measure_again;
}

#ifdef __cplusplus
}
#endif /* End of CPP guard */
