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

// Recommended mode of operation for weather monitoring
// Testing is needed, but I'd add 2x or 4x oversampling
// to temperature and humidity
static const struct bme280_settings default_settings = {
    .osr_p = BME280_OVERSAMPLING_1X,
    .osr_t = BME280_OVERSAMPLING_1X,
    .osr_h = BME280_OVERSAMPLING_1X,
    .filter = BME280_FILTER_COEFF_OFF,
};

// static struct bme280_dev dev = {};
// static struct bme280_data dev_data = {};
// static struct bme280_settings dev_settings = {};
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

static inline bool is_dev_initialised(const struct bme280_dev *dev)
{
    return !((dev->read == NULL) || (dev->write == NULL) || (dev->delay_us == NULL));
}

#define _BME280_HANDLER_PRINT_ERROR(err_code) print_error(err_code)

#define _BME280_HANDLER_CHECK_INIT(dev) \
do { \
    if (!is_dev_initialised(dev)) { \
        _BME280_HANDLER_PRINT_ERROR(BME280_HANDLER_NOT_INIT); \
        return false; \
    } \
} while (0)

bool bme280_handler_init(struct bme280_dev *dev,
                        i2c_inst_t *i2c_dev,
                        bool dont_i2c_init,
                        uint baudrate,
                        const struct bme280_settings *settings,
                        struct bme280_settings *resulting_settings)
{
    int8_t rslt;

    if(dont_i2c_init)
        rslt = bme280_init_i2c(dev, i2c_dev);
    else
        rslt = bme280_init_force_i2c(dev, i2c_dev, baudrate);
    _BME280_HANDLER_PRINT_ERROR(rslt);

    bool r1 = bme280_handler_set_sensor_settings(dev, BME280_SEL_ALL_SETTINGS, settings, resulting_settings);
    bool r2 = bme280_handler_cal_meas_delay_internal(dev, settings);

    return r1 && r2;
}

bool bme280_handler_set_sensor_settings(struct bme280_dev *dev,
                                        uint8_t desired_settings,
                                        const struct bme280_settings *settings,
                                        struct bme280_settings *resulting_settings)
{
    _BME280_HANDLER_CHECK_INIT(dev);

    const struct bme280_settings *settings_ptr = settings == NULL ? &default_settings : settings;

    int8_t rslt = bme280_set_sensor_settings(desired_settings, settings_ptr, &dev);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    bool r1 = rslt == BME280_OK;
    // if settings was set and resulting_settings is not NULL
    if (r1 && resulting_settings)
    {
        memcpy(resulting_settings, settings_ptr, sizeof(struct bme280_settings));
    }

    return r1;
}

bool bme280_handler_get_sensor_settings(struct bme280_dev *dev, struct bme280_settings *settings)
{
    _BME280_HANDLER_CHECK_INIT(dev);

    int8_t rslt = bme280_get_sensor_settings(settings, dev);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    return rslt == BME280_OK;
}

bool bme280_handler_set_sensor_mode(struct bme280_dev *dev, uint8_t sensor_mode)
{
    _BME280_HANDLER_CHECK_INIT(dev);

    int8_t rslt = bme280_set_sensor_mode(sensor_mode, dev);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    return rslt == BME280_OK;
}

bool bme280_handler_soft_reset(struct bme280_dev *dev)
{
    _BME280_HANDLER_CHECK_INIT(dev);

    int8_t rslt = bme280_soft_reset(dev);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    return rslt == BME280_OK;
}

bool bme280_handler_get_sensor_data(struct bme280_dev *dev, struct bme280_data *data)
{
    // have to externally check if enough time has passed between measurements
    _BME280_HANDLER_CHECK_INIT(dev);

    int8_t rslt = bme280_get_sensor_data(BME280_ALL, data, dev);

    _BME280_HANDLER_PRINT_ERROR(rslt);
    return rslt == BME280_OK;
}

bool bme280_handler_cal_meas_delay(const struct bme280_dev *dev, const struct bme280_settings *settings,
                                   uint32_t *max_delay_ms)
{
    _BME280_HANDLER_CHECK_INIT(dev);

    uint32_t max_delay_ms_temp;
    int8_t rslt = bme280_cal_meas_delay(&max_delay_ms_temp, settings);
    _BME280_HANDLER_PRINT_ERROR(rslt);
    bool res = rslt == BME280_OK;

    if (res)
        *max_delay_ms = max_delay_ms_temp;

    return res;
}

#ifdef __cplusplus
}
#endif /* End of CPP guard */
