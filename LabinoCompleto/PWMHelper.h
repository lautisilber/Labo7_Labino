#ifndef PWM_HELPER_H
#define PWM_HELPER_H

#include <Arduino.h>
// https://github.com/terryjmyers/PWM
// Con esta libreria podemos controlar exactamente la frecuencia del PWM sin tener que setear a mano los registros correspondientes
#include <PWM.h>

// La libreria tiene soporte para estas dos familias de MCU ATmega
// a) ATmega48/88/168/328,
// b) ATmega640/1280/1281/2560/2561

#if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
#define MCU_TIPO_MEGA
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define MCU_TIPO_UNO
#else
#error "MCU no soportado"
#endif

// Los pines asociados a los timers son de la siguiente manera. Fuente (https://github.com/khoih-prog/AVR_PWM?tab=readme-ov-file#1-timer0)
/******************************************************************************************************************************
  // For UNO / Nano
  Timer0 ( 8-bit) used by delay(), millis() and micros(), and PWM generation on pins 5 (6 not usable)
  Timer1 (16-bit) used by the Servo.h library and PWM generation on pins 9 and 10
  Timer2 ( 8-bit) used by Tone() and PWM generation on pins 3 and 11
  // For Mega
  Timer0 ( 8-bit) used by delay(), millis() and micros(), and PWM generation on pins 4 (13 not usable)
  Timer1 (16-bit) used by the Servo.h library and PWM generation on pins 11, 12
  Timer2 ( 8-bit) used by Tone() and PWM generation on pins 9 and 10
  Timer3 (16-bit) used by PWM generation on pins  2,  3 and  5
  Timer4 (16-bit) used by PWM generation on pins  6,  7 and  8
  Timer5 (16-bit) used by PWM generation on pins 44, 45 and 46

  ////////////////////////////////////////////
  // For Mega (2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 44, 45, 46)
  Pin  2 => TIMER3B   // PE 4 ** 2  ** PWM2
  Pin  3 => TIMER3C   // PE 5 ** 3  ** PWM3
  Pin  4 => TIMER0B   // PG 5 ** 4  ** PWM4
  Pin  5 => TIMER3A   // PE 3 ** 5  ** PWM5
  Pin  6 => TIMER4A   // PH 3 ** 6  ** PWM6
  Pin  7 => TIMER4B   // PH 4 ** 7  ** PWM7
  Pin  8 => TIMER4C   // PH 5 ** 8  ** PWM8
  Pin  9 => TIMER2B   // PH 6 ** 9  ** PWM9
  Pin 10 => TIMER2A   // PB 4 ** 10 ** PWM10
  Pin 11 => TIMER1A   // PB 5 ** 11 ** PWM11
  Pin 12 => TIMER1B   // PB 6 ** 12 ** PWM12
  Pin 13 => TIMER0A   // PB 7 ** 13 ** PWM13
  Pin 44 => TIMER5C   // PL 5 ** 44 ** D44
  Pin 45 => TIMER5B   // PL 4 ** 45 ** D45
  Pin 46 => TIMER5A   // PL 3 ** 46 ** D46
  ////////////////////////////////////////////
  // For UNO, Nano (3, 5, 6, 9, 10, 11)
  Pin  3 => TIMER2B,
  Pin  5 => TIMER0B
  Pin  6 => TIMER0A
  Pin  9 => TIMER1A
  Pin 10 => TIMER1B
  Pin 11 => TIMER2(A)
******************************************************************************************************************************/

// Con la libreria PWM.h en particular se testearon los siguientes pines exitosamente para Arduino UNO/Nano
// For UNO, Nano (3, 5, 6, 9, 10, 11)
//   Pin  3 => TIMER2B    TESTED
//   Pin  5 => TIMER0B    TESTED
//   Pin  6 => TIMER0A
//   Pin  9 => TIMER1A    TESTED
//   Pin 10 => TIMER1B    TESTED
//   Pin 11 => TIMER2(A)

// En resumen, para Arduinos tipo UNO usar preferentemente pin 3. NO usar pines 5 y 6
// para Arduinos tipo MEGA NO usar pines 4 y 13 (cualquier otro de la lista deberia estar bien)

namespace PWMHelper
{
    bool begin(byte pin)
    {
        // No se exactamente como funciona esto. Pareciera que digitalPinHasPWM() (definida en pins_arduino.h) devuelve un booleano
        // dependiendo de si el pin eseta vinculado a un Timer. La funcion digitalPinToTimer() (definida en pins_arduino.h) devuelve
        // el timer al que esta vinculado el pin. Este valor lo podemos comparar contra las constantes definidas TIMERXX para determinar
        // efectivamente cual es el timer vinculado a ese pin. La idea es inicializar unicamente el timer necesario

        if (!digitalPinHasPWM(pin))
            return false;
        uint8_t timer = digitalPinToTimer(pin);

    #ifdef MCU_TIPO_UNO
        if (timer == TIMER1A || timer == TIMER1B)
            Timer1_Initialize();
        else if (timer == TIMER2B)
            Timer2_Initialize();
    #elif defined(MCU_TIPO_MEGA)
        if(timer == TIMER1A || timer == TIMER1B)
            Timer1_Initialize();
        else if(timer == TIMER2B)
            Timer2_Initialize();
        else if(timer == TIMER3A || timer == TIMER3B || timer == TIMER3C)
            Timer3_Initialize();
        else if(timer == TIMER4A || timer == TIMER4B || timer == TIMER4C)
            Timer4_Initialize();
        else if(timer == TIMER5A || timer == TIMER5B || timer == TIMER5C)
            Timer5_Initialize();
    #endif
        else
            return false;
        return true;
    }

    inline bool setFrequency(byte pin, uint32_t frequency)
    {
        return SetPinFrequencySafe(pin, frequency);
    }

    inline void write(uint8_t pin, uint8_t val)
    {
        pwmWrite(pin, val);
    }

    inline void writeHR(uint8_t pin, uint16_t val)
    {
        pwmWriteHR(pin, val);
    }
}


// Good pins for PWM
// For UNO, Nano (3, 5, 6, 9, 10, 11)
//   Pin  3 => TIMER2B    TESTED
//   Pin  5 => TIMER0B    TESTED
//   Pin  6 => TIMER0A
//   Pin  9 => TIMER1A    TESTED
//   Pin 10 => TIMER1B    TESTED
//   Pin 11 => TIMER2(A)
#define DEFAULT_PWM_FREQUENCY 20000

constexpr uint8_t percent2dutyCycleF(float percent)
{
    return roundf( (percent / 100) * 255 );
}

constexpr uint8_t percent2dutyCycleI(int percent)
{
    return percent2dutyCycleF((float)percent);
}

class PWMPin
{
private:
    byte _pin;
    uint32_t _frequency;
    uint8_t _dutyCycle;

public:
    PWMPin(byte pin, uint8_t dutyCycle, uint32_t frequency=DEFAULT_PWM_FREQUENCY)
        : _pin(pin), _dutyCycle(dutyCycle), _frequency(frequency)
    {
        bool s1 = PWMHelper::begin(pin);
        bool s2 = setFrequency(pin, frequency);
        if (!(s1 && s2))
        {
            Serial.print(F("ERROR: No se pudo inicializar PWM en el pin "));
            Serial.println(pin);
        }
    }

    bool setFrequency(byte pin, uint32_t frequency)
    {
        return PWMHelper::setFrequency(pin, frequency);
    }

    void setDutyCycle(uint8_t dutyCycle)
    {
        // 0 = 0%, 256 = 100% of the PWM duty cycle that controls the pump
        _dutyCycle = dutyCycle;
    }

    void setPercent(uint8_t percent)
    {
        setDutyCycle(percent2dutyCycleI(percent));
    }

    void state(bool s)
    {
        uint8_t dc = _dutyCycle * (uint8_t)s;
        PWMHelper::write(_pin, dc);
    }
};


#endif