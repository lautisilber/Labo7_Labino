#include "i2c_helper.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h" // for picotool

#ifdef __cplusplus
extern "C" {
#endif

void i2c_init_helper_only_i2c(i2c_inst_t *i2c, uint baudrate)
{
    i2c_init(i2c, baudrate);
}

void i2c_init_helper_only_pins(uint sda_pin, uint scl_pin, bool pullups)
{
    // useful macros
    // PICO_DEFAULT_I2C_SDA_PIN: default SDA gpio pin #
    // PICO_DEFAULT_I2C_SCL_PIN: default SCL gpio pin #

    // for picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    if (pullups)
    {
        gpio_pull_up(sda_pin);
        gpio_pull_up(scl_pin);
    }
}

void i2c_init_helper_full(i2c_inst_t *i2c, uint baudrate, uint sda_pin, uint scl_pin, bool pullups)
{
    i2c_init_helper_only_i2c(i2c, baudrate);
    i2c_init_helper_only_pins(sda_pin, scl_pin, pullups);
}


#ifdef __cplusplus
}
#endif /* End of CPP guard */