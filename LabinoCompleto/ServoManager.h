#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include <Arduino.h>
#include <Servo.h> // para controlar el servo

#define SERVO_MIN_ANGLE 1
#define SERVO_MAX_ANGLE 179
#define SERVO_CENTER_ANGLE 90

class ServoManager
{
private:
    Servo _servo;
    byte _pin;
    uint16_t _delay_ms;
    uint8_t _angle = SERVO_CENTER_ANGLE;

    void goToAngleRaw(uint8_t angle)
    {
        if (angle < SERVO_MIN_ANGLE || angle > SERVO_MAX_ANGLE) return;
        _servo.write(angle);
    }

public:
    ServoManager(byte pin, unsigned long delay_ms=5)
        : _servo(), _pin(pin), _delay_ms(delay_ms)
    {}

    void begin()
    {
        _servo.attach(_pin, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
        goToAngleRaw(SERVO_CENTER_ANGLE);
    }

    bool angle(uint8_t angle)
    {
        if (angle < SERVO_MIN_ANGLE || angle > SERVO_MAX_ANGLE) return false;
        if (angle > _angle)
        {
            for (uint8_t i = _angle; i <= angle; i++)
            {
                _servo.write(i);
                delay(_delay_ms);
            }
        }
        else
        {
            for (uint8_t i = _angle; i >= angle; i--)
            {
                _servo.write(i);
                delay(_delay_ms);
            }
        }
        _angle = angle;
        return true;
    }
    void setDelay(unsigned long delay_ms) { _delay_ms = delay_ms; }

    uint8_t getAngle() const { return _angle; }
};

#endif