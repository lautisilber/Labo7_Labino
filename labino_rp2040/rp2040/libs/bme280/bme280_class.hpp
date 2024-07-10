#ifndef BME280_CLASS_HPP
#define BME280_CLASS_HPP

#include "pico/stdlib.h"
#include "bme280_handler.h"

#define BME280_CLASS_MIN_MEASUREMENT_DELAY_MS 50

class BME280_I2C
{
private:
    struct bme280_dev _dev;
    struct bme280_data _data;
    struct bme280_settings _settings;
    uint32_t _max_delay_ms = 0;

//// Alarms ////////////

// due to the sensor having to take time between measurements, we have to wait between polls.
// When we measure, we start an alarm that will wait the maximum required time between
// measurements give the sensor settings.

public:
    // this flag should not be accessed directly
    volatile bool _can_measure_flag = true;
    static int64_t _alarm_callback(alarm_id_t id, void *user_data);
private:
    bool _schedule_measurement_alarm_flag(uint32_t in_ms);
////////////////////////

private:
    bool calculate_measurement_delay();

public:
    bool begin(i2c_inst_t *i2c_dev=i2c0, bool dont_i2c_init=false, uint baudrate=400000,
               const struct bme280_settings *settings=NULL);

    bool set_sensor_settings(const struct bme280_settings *settings);

    bool get_sensor_settings(struct bme280_settings *settings)
    {
        return bme280_handler_get_sensor_settings(&_dev, settings);
    }

    /*
    *@verbatim
    *    sensor_mode       |      Macros
    * ---------------------|-------------------------
    *     0                | BME280_POWERMODE_SLEEP
    *     1                | BME280_POWERMODE_FORCED
    *     3                | BME280_POWERMODE_NORMAL
    *@endverbatim
    */
    bool set_sensor_mode(uint8_t sensor_mode);

    bool soft_reset();

    const bme280_data* get_last_sensor_data() const { return (const bme280_data*)&_data; }
    bool get_sensor_data(const bme280_data* data);
};

#endif /* BME280_CLASS_HPP */