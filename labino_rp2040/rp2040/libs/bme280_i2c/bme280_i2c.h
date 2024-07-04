#ifndef BME280_I2C_H
#define BME280_I2C_H

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bme280.h"

#ifndef BME280_I2C_DEFAULT
#define BME280_I2C_DEFAULT i2c0
#endif

#ifndef BME280_DEFAULT_I2C_SPEED
#define BME280_DEFAULT_I2C_SPEED 100000
#endif

#define BME280_I2C_BAD_REFERENCE_SET                  INT8_C(-7)
#define BME280_I2C_COULDNT_SET_BAUDRATE               INT8_C(-8)

int8_t bme280_init_i2c(struct bme280_dev *dev, i2c_inst_t *i2c);
int8_t bme280_init_force_i2c(struct bme280_dev *dev, i2c_inst_t *i2c, uint baudrate); // this also initializes i2c

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* BME280_I2C_H */
