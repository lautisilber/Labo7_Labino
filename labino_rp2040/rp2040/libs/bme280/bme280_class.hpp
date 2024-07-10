#ifndef BME280_CLASS_HPP
#define BME280_CLASS_HPP

#include "pico/stdlib.h"
#include "bme280.h"

/*
    CAN ONLY BE USED WITH ONE I2C INTERFACE
*/

#define BME280_STOMASENSE_SDA_PIN                     8
#define BME280_STOMASENSE_SCL_PIN                     9
#define BME280_STOMASENSE_I2C_INTERFACE               i2c0
#define BME280_CLASS_MIN_MEASUREMENT_DELAY_MS         50

#define BME280_I2C_BAD_REFERENCE_SET                  INT8_C(-7)
#define BME280_I2C_COULDNT_SET_BAUDRATE               INT8_C(-8)
#define BME280_CLASS_NOT_INIT                         INT8_C(-9)
#define BME280_CLASS_NOT_ENOUGH_TIME_BETWEEN_READS    INT8_C(-10)

class BME280_I2C
{
private:
    struct bme280_dev _dev = {0};
    struct bme280_data _data = {0};
    struct bme280_settings _settings = {0};
    uint32_t _max_delay_ms = 0;
    bool _init_flag = false;
private: // other data
    uint _baudrate;
    uint _sda_pin, _scl_pin;

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
    BME280_I2C(uint sda_pin=BME280_STOMASENSE_SDA_PIN, uint scl_pin=BME280_STOMASENSE_SCL_PIN, uint baudrate=400000,
               const struct bme280_settings *settings=NULL);
    bool begin(bool dont_i2c_init=false, bool pullups=false);

    bool set_sensor_settings(const struct bme280_settings *settings);

    bool get_sensor_settings(struct bme280_settings *settings);

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

    uint32_t get_measurement_delay() const { return _max_delay_ms; }
    bool can_measure() const { return _can_measure_flag; }
};

#endif /* BME280_CLASS_HPP */