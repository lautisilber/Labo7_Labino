#include "bme280_i2c.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdbool.h>

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

static i2c_inst_t *i2c_dev = NULL;

// typedef BME280_INTF_RET_TYPE (*bme280_read_fptr_t)(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
static BME280_INTF_RET_TYPE bme280_read_i2c(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    // int i2c_read_blocking_until (i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop, absolute_time_t until)
    const bool nonstop = false; // if false, release bus
    const uint timeout_us = 10 * 1000 * 1000; // 10 s
    int rslt = i2c_read_timeout_us(i2c_dev, reg_addr, reg_data, len, nonstop, timeout_us);
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
    const bool nonstop = false; // if false, release bus
    const uint timeout_us = 10 * 1000 * 1000; // 10 s
    int rslt = i2c_write_timeout_us(i2c_dev, reg_addr, reg_data, len, nonstop, timeout_us);
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

/******************************************************************************/
/*!                      Internal functions                                   */

static inline void bme280_init_struct_callbacks(struct bme280_dev *dev)
{
    dev->intf = BME280_I2C_INTF;
    dev->read = bme280_read_i2c;
    dev->write = bme280_write_i2c;
    dev->delay_us = bme280_delay_us_i2c;
}

static bool i2c_initialised()
{
    return i2c_dev == i2c0 || i2c_dev == i2c1;
}

static int8_t bme280_internal_i2c_init(struct bme280_dev *dev)
{
    int8_t init_res;

    init_res = bme280_init(dev);
    if (init_res != BME280_OK)
        return init_res;

    return BME280_OK;
}

/******************************************************************************/
/*!                User interface functions                                   */

int8_t bme280_init_i2c(struct bme280_dev *dev, i2c_inst_t *i2c)
{
    int8_t init_res;

    i2c_dev = i2c == NULL ? BME280_I2C_DEFAULT : i2c;

    if (!i2c_initialised())
        return BME280_I2C_BAD_REFERENCE_SET;

    return bme280_internal_i2c_init(dev);
}

int8_t bme280_init_force_i2c(struct bme280_dev *dev, i2c_inst_t *i2c, uint baudrate)
{
    i2c_dev = i2c == NULL ? BME280_I2C_DEFAULT : i2c;

    if (!i2c_initialised())
        return BME280_I2C_BAD_REFERENCE_SET;

    // uint i2c_init (i2c_inst_t *i2c, uint baudrate)
    uint rslt = i2c_init(i2c_dev, baudrate > 0 ? baudrate : BME280_DEFAULT_I2C_SPEED);
    if (rslt != baudrate)
        return BME280_I2C_COULDNT_SET_BAUDRATE;

    return bme280_internal_i2c_init(dev);
}


#ifdef __cplusplus
}
#endif /* End of CPP guard */
