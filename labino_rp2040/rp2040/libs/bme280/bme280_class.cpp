#include "bme280_class.hpp"


int64_t BME280_I2C::_alarm_callback(alarm_id_t id, void *user_data)
{
    // TODO: check that this variable is being changed atomicaly
    ((BME280_I2C*)user_data)->_can_measure_flag = true;
    return 0; // do not reschedule alarm automaticaly
}

bool BME280_I2C::_schedule_measurement_alarm_flag(uint32_t in_ms)
{
    if (_can_measure_flag < 0) return false;
    alarm_id_t id = add_alarm_in_ms(in_ms, _alarm_callback, this, true);
    return id >= 0;
}

bool BME280_I2C::calculate_measurement_delay()
{
    return bme280_handler_cal_meas_delay(&_dev, &_settings, &_max_delay_ms);
}

bool BME280_I2C::begin(i2c_inst_t *i2c_dev=i2c0, bool dont_i2c_init=false, uint baudrate=400000, const struct bme280_settings *settings=NULL)
{
    bool res = bme280_handler_init(&_dev, i2c_dev, dont_i2c_init, baudrate, settings, &_settings);
    if (!res)
        return false;
    return calculate_measurement_delay();
}

bool BME280_I2C::set_sensor_settings(const struct bme280_settings *settings)
{
    bool res = bme280_handler_set_sensor_settings(&_dev, BME280_SEL_ALL_SETTINGS, settings, &_settings);
    if (!res)
        return false;
    return calculate_measurement_delay();
}

bool BME280_I2C::get_sensor_settings(struct bme280_settings *settings)
{
    return bme280_handler_get_sensor_settings(&_dev, settings);
}

bool BME280_I2C::set_sensor_mode(uint8_t sensor_mode)
{
    return bme280_handler_set_sensor_mode(&_dev, sensor_mode);
}

bool BME280_I2C::soft_reset()
{
    bool bme280_handler_soft_reset(&_dev);
}

bool BME280_I2C::get_sensor_data(const bme280_data* data)
{
    if (!_can_measure_flag)
    {
        data = nullptr;
        return false;
    }

    bme280_handler_get_sensor_data(&_dev, &_data);
    _can_measure_flag = false;
    bool alarm_scheduled = _schedule_measurement_alarm_flag(
        (_max_delay_ms > 0 ? _max_delay_ms : BME280_CLASS_MIN_MEASUREMENT_DELAY_MS) + 10); // add 10 milliseconds, just in case and clamp minimum if delay is 0
    if (!alarm_scheduled) _can_measure_flag = true; // a fallback to avoid softlocking

    return get_last_sensor_data();
}