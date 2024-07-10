#include "bme280_defs.h"
#include "bme280_i2c.h"
#include <stdbool.h>
#include "hardware/i2c.h"

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

#define BME280_HANDLER_NOT_INIT                         INT8_C(-9)
#define BME280_HANDLER_NOT_ENOUGH_TIME_BETWEEN_READS    INT8_C(-10)

// for default: bme280_handler_init(NULL, false, 0, NULL);
bool bme280_handler_init(i2c_inst_t *i2c_dev,
                        bool dont_i2c_init,
                        uint baudrate,
                        const struct bme280_settings *settings);
/*
*@verbatim
* Macros                 |   Functionality
* -----------------------|----------------------------------------------
* BME280_SEL_OSR_PRESS   |   To set pressure oversampling.
* BME280_SEL_OSR_TEMP    |   To set temperature oversampling.
* BME280_SEL_OSR_HUM     |   To set humidity oversampling.
* BME280_SEL_FILTER      |   To set filter setting.
* BME280_SEL_STANDBY     |   To set standby duration setting.
* BME280_SEL_ALL_SETTINGS|   To set all settings.
*@endverbatim
*/
bool bme280_handler_set_sensor_settings(uint8_t desired_settings,
                                  const struct bme280_settings *settings);
bool bme280_handler_get_sensor_settings(struct bme280_settings *settings);
/*
*@verbatim
*    sensor_mode       |      Macros
* ---------------------|-------------------------
*     0                | BME280_POWERMODE_SLEEP
*     1                | BME280_POWERMODE_FORCED
*     3                | BME280_POWERMODE_NORMAL
*@endverbatim
*/
bool bme280_handler_set_sensor_mode(uint8_t sensor_mode);
bool bme280_handler_get_sensor_mode(uint8_t *sensor_mode);
bool bme280_handler_soft_reset();
const struct bme280_data* bme280_handler_get_sensor_data();
bool bme280_handler_cal_meas_delay(uint32_t *max_delay_ms, bool force_update);
bool bme280_handler_can_measure();

#ifdef __cplusplus
}
#endif /* End of CPP guard */
