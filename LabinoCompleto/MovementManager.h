#ifndef MOVEMENT_MANAGER_H
#define MOVEMENT_MANAGER_H

// TODO:
/*
    Tal vez sea mejor usar un snsor de distancia (ya sesa optico o sonico) para determinar la posicion del stepper
    Esto es por dos razones:
        - Es mucho mas sencillo de prender/apagar dado que el sistema no necesita confiar que comienza en la posicion 0
        - Es posible que si se deja corriendo por mucho tiempo, el stepper experimente un corrimiento y se descalibren las posiciones
    
    Tampoco seria una pesima idea adquirir un boton (que funcione como la impresora 3D) que apriete el sistema de riego
    solo cuando este en la posicion 0. de esta manera se puede auto-calibrar la posicion del sistema de riego

    Cuando sepa que pin exactamente voy a utilizar para la bomba, seria prudente reemplazar la funcion de la libreria PWM "InitTimersSafe"
    por alguna de las funciones "Timer<N>_Initialize" donde <N> es el indice del timer. Estaria bueno solo activar el timer responsable de
    llevar a cabo el PWM del pin que se utilice, y no todos.
*/

#include <Arduino.h>

// https://www.makerguides.com/28byj-48-stepper-motor-arduino-tutorial
// https://github.com/arduino-libraries/Stepper
// #include <Stepper.h>
#include "ULN2003.h"
#include "ServoManager.h"
#include "PROGMEMUtils.h"


// https://lastminuteengineers.com/28byj48-stepper-motor-arduino-tutorial/#
#define STEPS_PER_REVOLUTION_28BYJ_48_5V 2048
#define MAX_MACETAS 32
#define MOVEMENT_MANAGER_ERROR_LOG_STR_MAX_SIZE 128

// constexpr inline uint16_t map8to16(const uint8_t u8)
// {
//     return u8 << 8; // this is effectively mapping from (0, 255) -> (0, 65280)
// }

class MovementManager
{
private:
    byte _pinIN1, _pinIN2, _pinIN3, _pinIN4;
    byte _pinServo;

    ULN2003 _stepper;
    ServoManager _servo;

    long _stepperSpeed; // ms per revolution (fastest is approx 5 s = 5000 ms)
    uint16_t _servoSpeed; // delay in ms between each angle

    char _errorStr[MOVEMENT_MANAGER_ERROR_LOG_STR_MAX_SIZE] = {0};

public:
    MovementManager(byte in1, byte in2, byte in3, byte in4, byte servo, long stepperSpeed=5000, uint16_t servoSpeed=15)
        : _stepper(ULN2003(in1, in2, in3, in4, 2000)), _servo(servo, servoSpeed),
          _pinIN1(in1), _pinIN2(in2), _pinIN3(in3), _pinIN4(in4), _pinServo(servo), _stepperSpeed(stepperSpeed), _servoSpeed(servoSpeed)
    {
        // set stepper speed
        setStepperSpeed(stepperSpeed);
    }

    bool begin()
    {
        _servo.begin();
    }

    /// stepper ///
    inline void setStepperSpeed(long stepperSpeed) { _stepperSpeed = stepperSpeed; _stepper.setMsPerRevolution(stepperSpeed); }
    bool stepperGoToStep(long step, bool detach=false)
    {
        if (step < 0)
        {
            SNPRINTF_FLASH(_errorStr, MOVEMENT_MANAGER_ERROR_LOG_STR_MAX_SIZE-1, F("ERROR: No se puede ir a una posicion negativa. La posicion provista fue %i"), step);
            return false;
        }
        _stepper.goToPosition(step);
        if (detach)
            stepperAttach(false);
        return true;
    }
    void stepperAttach(bool attach) { _stepper.attach(attach); }
    inline long getStepperStep() const { return _stepper.getCurrentPosition(); }
    //////////////

    /// servo ///
    bool servoGoToAngle(uint8_t angle, bool detach=false)
    {
        if (!_servo.attached())
            _servo.attach(true);
        bool res = _servo.angle(angle);
        if (detach)
            servoAttach(false);
        if (!res)
        {
            SNPRINTF_FLASH(_errorStr, MOVEMENT_MANAGER_ERROR_LOG_STR_MAX_SIZE-1, F("ERROR: El servo no puede ir angulo %u, ya que se puede mover entre %u y %u"), angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
            return false;
        }
        return true;
    }
    void servoAttach(bool attach) { _servo.attach(attach); }
    inline uint8_t getServoAngle() const { return _servo.getAngle(); }
    /////////

    inline void printError(Stream *stream)
    {
        stream->println(_errorStr);
        memset(_errorStr, '\0', MOVEMENT_MANAGER_ERROR_LOG_STR_MAX_SIZE);
    }
};


#endif