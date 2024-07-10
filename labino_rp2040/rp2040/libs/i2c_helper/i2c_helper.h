#ifndef I2C_HELPER_H
#define I2C_HELPER_H

#include "hardware/i2c.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void i2c_init_helper_only_i2c(i2c_inst_t *i2c, uint baudrate);
void i2c_init_helper_only_pins(uint sda_pin, uint scl_pin, bool pullups);

void i2c_init_helper_full(i2c_inst_t *i2c, uint baudrate, uint sda_pin, uint scl_pin, bool pullups);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* I2C_HELPER_H */