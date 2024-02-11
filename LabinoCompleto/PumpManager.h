#ifndef PUMP_MANAGER_H
#define PUMP_MANAGER_H

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

// https://github.com/terryjmyers/PWM
// Con esta libreria podemos controlar exactamente la frecuencia del PWM sin tener que setear a mano los registros correspondientes
// #include <PWM.h>
#include "PWMHelper.h"


// https://lastminuteengineers.com/28byj48-stepper-motor-arduino-tutorial/#
#define STEPS_PER_REVOLUTION_28BYJ_48_5V 2048
#define MAX_MACETAS 32
#define PUMP_MANAGER_ERROR_LOG_STR_MAX_SIZE 128

// Good pins for PWM
// For UNO, Nano (3, 5, 6, 9, 10, 11)
//   Pin  3 => TIMER2B    TESTED
//   Pin  5 => TIMER0B    TESTED
//   Pin  6 => TIMER0A
//   Pin  9 => TIMER1A    TESTED
//   Pin 10 => TIMER1B    TESTED
//   Pin 11 => TIMER2(A)
#define DEFAULT_PWM_FREQUENCY 20000

// constexpr inline uint16_t map8to16(const uint8_t u8)
// {
//     return u8 << 8; // this is effectively mapping from (0, 255) -> (0, 65280)
// }

typedef struct Position
{
    long step;
    bool angle; // true is one side of the system, false is the other
} Position;

template <size_t N>
class PumpManager
{
private:
    byte _pinIN1, _pinIN2, _pinIN3, _pinIN4;
    byte _pinServo;
    byte _pinPump;

    ULN2003 _stepper;
    ServoManager _servo;

    Position *_positions;
    long _stepperSpeed; // ms per revolution (fastest is approx 5 s = 5000 ms)
    uint16_t _servoSpeed; // delay in ms between each angle
    uint8_t _pumpSpeed; // 0 = 0%, 256 = 100% of the PWM duty cycle that controls the pump

    char _errorStr[PUMP_MANAGER_ERROR_LOG_STR_MAX_SIZE] = {0};

    inline void _setStepperSpeedOnly(long stepperSpeed) { _stepper.setMsPerRevolution(stepperSpeed); }
    bool _setPWMFrequency(uint32_t frequency)
    {
        //sets the frequency for the specified pin
	    bool success = PWMHelper::setFrequency(_pinPump, frequency); // de la libreria PWM

        if (!success) {
            snprintf(_errorStr, PUMP_MANAGER_ERROR_LOG_STR_MAX_SIZE-1, "ERROR: No se pudo setear la frecuencia PWM del pin %i a la frecuencia %i", _pinPump, frequency);
        }

        return success;
    }

public:
    PumpManager(byte in1, byte in2, byte in3, byte in4, byte servo, byte pump, const Position positions[N], long stepperSpeed=5000, uint16_t servoSpeed=15, uint8_t pumpSpeed=percent2dutyCycleI(50))
        : _stepper(ULN2003(in1, in2, in3, in4, 2000)), _servo(servo, servoSpeed),
          _pinIN1(in1), _pinIN2(in2), _pinIN3(in3), _pinIN4(in4), _pinServo(servo), _pinPump(pump), _stepperSpeed(stepperSpeed), _servoSpeed(servoSpeed), _pumpSpeed(pumpSpeed)
    {
        // set positions
        _positions = positions;
        
        // set stepper speed
        _setStepperSpeedOnly(stepperSpeed);
    }

    bool begin()
    {
        const bool s1 = PWMHelper::begin(_pinPump);
        const bool s2 = _setPWMFrequency(_pumpSpeed); // de la libreria PWM
        _servo.begin();
        return s1 && s2;
    }

    void pumpState(bool state) // AAAAAA
    {
        const uint8_t dutyCycle = state ? _pumpSpeed : 0;
        PWMHelper::write(_pinPump, dutyCycle); // de la libreria PWM
    }
    inline void pumpOn() { pumpState(true); }
    inline void pumpOff() { pumpState(false); }
    void pumpForTime(unsigned long time_ms)
    {
        pumpOn();
        delay(time_ms);
        pumpOff();
    }


    inline bool setPumpSpeed(uint8_t pumpSpeed, bool force=false) { _pumpSpeed = pumpSpeed; }
    inline void setStepperSpeed(long stepperSpeed) { _stepperSpeed = stepperSpeed; _setStepperSpeedOnly(stepperSpeed); }

    bool stepperGoToStep(long step)
    {
        if (step < 0)
        {
            snprintf(_errorStr, PUMP_MANAGER_ERROR_LOG_STR_MAX_SIZE-1, "ERROR: No se puede ir a una posicion negativa. La posicion provista fue %i", step);
            return false;
        }
        _stepper.goToPosition(step);
        return true;
    }
    bool stepperGoToPosition(size_t positionIndex)
    {
        if (positionIndex >= N)
        {
            snprintf(_errorStr, PUMP_MANAGER_ERROR_LOG_STR_MAX_SIZE-1, "ERROR: El stepper no puede ir a la posicion %i ya que hay unicamente %i posiciones", positionIndex, N);
            return false;
        }

        return stepperGoToStep(_positions[positionIndex].step);
    }
    bool stepperGoHome() { return stepperGoToPosition(0); }

    bool servoGoToAngle(uint8_t angle)
    {
        bool res = _servo.angle(angle);
        if (!res)
        {
            snprintf(_errorStr, PUMP_MANAGER_ERROR_LOG_STR_MAX_SIZE-1, "ERROR: El servo no puede ir al angulo %u", angle);
            return false;
        }
        return true;
    }
    bool servoGoToPosition(size_t positionIndex)
    {
        if (positionIndex >= N)
        {
            snprintf(_errorStr, PUMP_MANAGER_ERROR_LOG_STR_MAX_SIZE-1, "ERROR: El servo no puede ir a la posicion %i  ya que hay unicamente %i posiciones", positionIndex, N);
            return false;
        }

        uint8_t angle = _positions[positionIndex].angle ? SERVO_MAX_ANGLE : SERVO_MIN_ANGLE;
        return servoGoToAngle(angle);
    }
    bool servoGoHome() { return servoGoToAngle(SERVO_CENTER_ANGLE); }

    bool water(size_t positionIndex, unsigned long time_ms, bool returnHome=true)
    {
        if(!stepperGoToPosition(positionIndex) || !servoGoToPosition(positionIndex));
            return false;
        
        pumpForTime(time_ms);

        if (returnHome)
            return stepperGoHome() && servoGoHome();
        return true;
    }
    bool water(size_t positionIndex, unsigned long time_ms, uint8_t pumpSpeed, bool returnHome=true)
    {
        uint8_t oldPumpSpeed = _pumpSpeed;
        setPumpSpeed(_pumpSpeed);
        bool res = water(positionIndex, time_ms, returnHome);
        _pumpSpeed = oldPumpSpeed;
        return res;
    }

    inline void printError(Stream *stream)
    {
        stream->println(_errorStr);
        memset(_errorStr, '\0', PUMP_MANAGER_ERROR_LOG_STR_MAX_SIZE);
    }
    inline long getStepperStep() const { return _stepper.getCurrentPosition(); }
    inline uint8_t getServoAngle() const { return _servo.getAngle(); }
};

constexpr uint8_t percent2dutyCycleF(float percent)
{
    return roundf( (percent / 100) * 255 );
}

constexpr uint8_t percent2dutyCycleI(int percent)
{
    return percent2dutyCycleF((float)percent);
}


#endif