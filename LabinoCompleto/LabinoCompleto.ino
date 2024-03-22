// https://github.com/adafruit/DHT-sensor-library
#include <DHT.h>

#include "SmartSerial.h"
#include "Balanzas.h"
#include "MovementManager.h"
#include "PWMHelper.h"

#define RCV_COMMAND "rcv"
#define BAUD_RATE 9600
#define SOFT_SERIAL false
#define ARR_LEN(a) sizeof(a)/sizeof(a[0])

#if SOFT_SERIAL
#include <SoftwareSerial.h>
#endif

// pins
const byte rxPin = 5, txPin = 6;
const byte sckPin = 2;
const byte dataPins[] = {A0, A1, A2, A3, A4, A5};
const byte dhtPin = 4;
const byte driverPins[4] = {8, 9, 10, 11}; // in1, in2, in3, in4
const byte servoPin = 7;
const byte pumpPin = 3;

const size_t nBalanzas = ARR_LEN(dataPins);

#define DHT_TYPE DHT22
DHT dht(dhtPin, DHT_TYPE);

#if SOFT_SERIAL
SoftwareSerial ser(rxPin, txPin);
#endif

MultipleHX711<nBalanzas> hx711(dataPins, sckPin);

MovementManager movement(
    driverPins[0], driverPins[1], driverPins[2], driverPins[3], // stepper driver pins
    servoPin, // pin servo
    1000, 20  // stepper speed (ms per revolution), servo speed (delay in ms between each angle), pump speed (0 = 0%, 256 = 100% of the PWM duty cycle that controls the pump)
);

PWMPin pump(pumpPin, percent2dutyCycleI(50));

void pumpForTime(unsigned long tiempo, uint8_t intensidad);

#define LED_ON() digitalWrite(LED_BUILTIN, HIGH)
#define LED_OFF() digitalWrite(LED_BUILTIN, LOW)

// if the command execution can take some time, the word "rcv" is sent before the execution to
// notify the command has been received and processed
void rcv(Stream *stream) { stream->println(F(RCV_COMMAND)); }
void cmdUnrecognized(Stream *stream, const char* cmd)
{
    LED_ON();
    stream->print(F("ERROR: No se reconoce el comando \""));
    stream->print(cmd);
    stream->println(F("\""));
    LED_OFF();
}

void cmdHX(Stream *stream, CommandArguments *comArgs)
{
    // cmd: hx <?int:n>
    // respuesta: [12,34,56,78,...]
    // devuelve los datos de la balanza

    // devuelve todas las balanzas

    LED_ON();

    uint8_t n;
    if (comArgs->N == 0)
    {
        n = 1;
    }
    else
    {
        // check if arg 1 is a number
        long arg;
        bool isInt = comArgs->toInt(0, &arg);

        if (!isInt)
        {
            stream->print(F("ERROR: El argumento 1 no es un numero entero. El argumento es "));
            stream->println(comArgs->arg(0));
            LED_OFF();
            return;
        }

        if (arg < 1 || arg > 255)
        {
            stream->print(F("ERROR: El argumento 1 debe ser un numero entre 1 y 255. El argumento es "));
            stream->println(arg);
            LED_OFF();
            return;
        }

        n = static_cast<uint8_t>(arg);
    }
    
    rcv(stream);

    float values[nBalanzas];
    bool s = hx711.readAvg(values, n, 1000);
    if (!s)
    {
        stream->println(F("ERROR: No se pudo leer las balanzas"));
        LED_OFF();
        return;
    }
    stream->print('[');
    for (size_t i = 0; i < nBalanzas; i++)
    {
        stream->print(values[i],4);
        if (i < nBalanzas-1)
            stream->print(',');
        else
            stream->println(']');
    }
    LED_OFF();
}

void cmdHXSingle(Stream *stream, CommandArguments *comArgs)
{
    // cmd: hx_single <int:n> <int:n>
    // respuesta: 56
    // devuelve los datos de la balanza

    // devuelve la balanza del indice unicamente

    LED_ON();

    if (comArgs->N < 2)
    {
        stream->println(F("ERROR: No se proporcinaron dos argumentos numericos."));
        LED_OFF();
        return;
    }

    // extract argument 1
    uint8_t n;
    {
        // check if arg 1 is a number
        long arg;
        bool isInt = comArgs->toInt(0, &arg);

        if (!isInt)
        {
            stream->print(F("ERROR: El argumento 1 no es un numero entero. El argumento es "));
            stream->println(comArgs->arg(0));
            LED_OFF();
            return;
        }

        if (arg < 1 || arg > 255)
        {
            stream->print(F("ERROR: El argumento 1 debe ser un numero entre 1 y 255. El argumento es "));
            stream->println(arg);
            LED_OFF();
            return;
        }

        n = static_cast<uint8_t>(arg);
    }

    // extract argument 2
    size_t index;
    {
        long arg;
        bool isInt = comArgs->toInt(1, &arg);

        if (!isInt)
        {
            stream->print(F("ERROR: El argumento 2 no es un numero entero. El argumento es "));
            stream->println(comArgs->arg(0));
            LED_OFF();
            return;
        }

        if (arg < 0 || arg > nBalanzas)
        {
            stream->print(F("ERROR: El argumento 2 debe ser un indice entre 0 y "));
            stream->print(nBalanzas-1);
            stream->print(F(". El argumento es "));
            stream->println(arg);
            LED_OFF();
            return;
        }

        index = static_cast<size_t>(arg);
    }
    
    rcv(stream);

    float value;
    bool s = hx711.readAvgSingle(index, &value, n);
    if (!s)
    {
        stream->println(F("ERROR: No se pudo leer las balanzas"));
        LED_OFF();
        return;
    }
    stream->println(value,4);

    LED_OFF();
}

void cmdNhx(Stream *stream, CommandArguments *comArgs)
{
    // cmd: hx_n
    // respuesta: la cantidad de balanzas

    LED_ON();
    stream->println(nBalanzas);
    LED_OFF();
}

void cmdDHT(Stream *stream, CommandArguments *comArgs)
{
    // cmd: dht
    // respuesta: {"hum":12.34,"temp":56.78}
    // devuelve datos del DHT

    LED_ON();
    float hum = dht.readHumidity();
    float temp = dht.readTemperature();

    stream->print(F("{\"hum\":"));
    stream->print(hum);
    stream->print(F(",\"temp\":"));
    stream->print(temp);
    stream->println(F("}"));
    LED_OFF();
}

void cmdStepper(Stream *stream, CommandArguments *comArgs)
{
    // cmd: stepper <int:steps> <bool:detach>
    // respuesta: <int:steps>
    // Devuelve la cantidad de pasos dados
    // Si se transmitio un argumento, este debe ser uint32_t y es el paso al que se debe llevar el stepper

    LED_ON();
    if (comArgs->N < 0)
    {
        stream->println(F("ERROR: No se proporciono un argumento numerico."));
        LED_OFF();
        return;
    }

    // check if is a number
    long steps;
    bool isInt = comArgs->toInt(0, &steps);

    if (!isInt)
    {
        stream->print(F("ERROR: El primer argumento no es un numero entero. El argumento es "));
        stream->println(comArgs->arg(0));
        LED_OFF();
        return;
    }

    bool detach = false;
    if (comArgs->N > 1)
    {
        // check if is a bool
        bool isBool = comArgs->toBool(1, &detach);

        if (!isBool)
        {
            stream->print(F("ERROR: El segundo argumento no es un valor booleano. El argumento es "));
            stream->println(comArgs->arg(1));
            LED_OFF();
            return;
        }
    }

    rcv(stream);
    bool success = movement.stepperMoveSteps(steps, detach);
    if (!success)
    {
        movement.printError(stream);
        movement.printError(stream);
        LED_OFF();
        return;
    }

    stream->println(detach ? F("1") : F("0"));
    LED_OFF();
}

void cmdServo(Stream *stream, CommandArguments *comArgs)
{
    // cmd: servo ?<int:angulo>
    // respuesta: <int:angulo>
    // si se especifica un angulo, se movera el servo hasta ese angulo. en cualquier
    // caso, devuelve el angulo final del servo

    LED_ON();
    if (comArgs->N > 0)
    {
        // check if is all numbers
        long angle;
        bool isInt = comArgs->toInt(0, &angle);
        if (!isInt)
        {
            stream->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
            stream->println(comArgs->arg(0));
            LED_OFF();
            return;
        }
        else if (angle < SERVO_MIN_ANGLE || angle > SERVO_MAX_ANGLE) // puse && en vez de || me quiero morir :(
        {
            stream->print(F("ERROR: El argumento es un numero menor a "));
            stream->print(SERVO_MIN_ANGLE);
            stream->print(F(" o mayor a "));
            stream->print(SERVO_MAX_ANGLE);
            stream->print(F(". El numero del argumento es "));
            stream->println(angle);
            LED_OFF();
            return;
        }

        rcv(stream);
        movement.servoGoToAngle(angle);
    }

    stream->println(movement.getServoAngle());
    LED_OFF();
}

void cmdPump(Stream *stream, CommandArguments *comArgs)
{
    // cmd: pump <int:tiempo> <int:intensidad>
    // respuesta: OK

    LED_ON();
    if (comArgs->N < 2)
    {
        stream->println(F("ERROR: No se proporcinaron dos argumentos numericos."));
        LED_OFF();
        return;
    }

    // check if it is a number
    long tiempo;
    bool isIntTiempo = comArgs->toInt(0, &tiempo);
    if (!isIntTiempo)
    {
        stream->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
        stream->println(comArgs->arg(0));
        LED_OFF();
        return;
    }

    long intensidadArg;
    bool isIntIntensidad = comArgs->toInt(1, &intensidadArg);
    if (!isIntIntensidad)
    {
        stream->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
        stream->println(comArgs->arg(1));
        LED_OFF();
        return;
    }
    if (intensidadArg < 0 || intensidadArg > 255) // y por que aca si puse || y no &&??? >:(
    {
        stream->print(F("ERROR: El argumento debe ser un numero entre 0 y 255. El argumento es "));
        stream->println(intensidadArg);
        LED_OFF();
        return;
    }
    uint8_t intensidad = static_cast<uint8_t>(intensidadArg);

    rcv(stream);
    pumpForTime(tiempo, intensidad);

    stream->println(F("OK"));
}

void cmdStepperAttach(Stream *stream, CommandArguments *comArgs)
{
    // cmd: stepper_attach <bool:attach>
    // respuesta: OK
    // Si attach es 1, fija el stepper a la posicion actual. Si attach es 0, suelta el stepper

    LED_ON();
    if (comArgs->N == 0)
    {
        stream->println(F("ERROR: No se proporcino un argumento booleano."));
        LED_OFF();
        return;
    }

    // check if is a bool
    bool attach;
    bool isBool = comArgs->toBool(0, &attach);

    if (!isBool)
    {
        stream->print(F("ERROR: El argumento no es un valor booleano. El argumento es "));
        stream->println(comArgs->arg(0));
        LED_OFF();
        return;
    }

    rcv(stream);
    movement.stepperAttach(attach);

    stream->println(attach ? F("1") : F("0"));
    LED_OFF();
}

void cmdServoAttach(Stream *stream, CommandArguments *comArgs)
{
    // cmd: servo_attach <bool:attach>
    // respuesta: OK
    // Si attach es 1, fija el servo a la posicion actual. Si attach es 0, suelta el servo

    LED_ON();
    if (comArgs->N == 0)
    {
        stream->println(F("ERROR: No se proporcino un argumento booleano."));
        LED_OFF();
        return;
    }

    // check if is a number
    bool attach;
    bool isBool = comArgs->toBool(0, &attach);

    if (!isBool)
    {
        stream->print(F("ERROR: El argumento no es un valor booleano. El argumento es "));
        stream->println(comArgs->arg(0));
        LED_OFF();
        return;
    }

    rcv(stream);
    movement.servoAttach(attach);

    stream->println(attach ? F("1") : F("0"));
    LED_OFF();
}

void cmdOK(Stream *stream, CommandArguments *comArgs)
{
    // cmd: ok
    // respuesta: OK

    LED_ON();
    stream->println(F("OK"));
    delay(500);
    LED_OFF();
}

#if SOFT_SERIAL
SmartSerial ss(&ser);
#else
SmartSerial ss(&Serial);
#endif

CreateSmartCommandF(cmdHX_, "hx", cmdHX); // equivalent to: const PROGMEM char com_hx[] = "hx"; SmartCommandF cmdHX_(com_hx, cmdHX);
CreateSmartCommandF(cmdHXSingle_, "hx_single", cmdHXSingle);
CreateSmartCommandF(cmdNhx_, "hx_n", cmdNhx);
CreateSmartCommandF(cmdDHT_, "dht", cmdDHT);
CreateSmartCommandF(cmdStepper_, "stepper", cmdStepper);
CreateSmartCommandF(cmdServo_, "servo", cmdServo);
CreateSmartCommandF(cmdPump_, "pump", cmdPump);
CreateSmartCommandF(cmdStepperAttach_, "stepper_attach", cmdStepperAttach);
CreateSmartCommandF(cmdServoAttach_, "servo_attach", cmdServoAttach);
CreateSmartCommandF(cmdOK_, "ok", cmdOK);

void setup()
{
    #if SOFT_SERIAL
    ser.begin(BAUD_RATE);
    #else
    Serial.begin(BAUD_RATE);
    #endif

    pinMode(LED_BUILTIN, OUTPUT);
    LED_ON();

    movement.begin();
    hx711.begin();

    dht.begin();

    ss.setDefaultCallback(cmdUnrecognized);
    ss.addCommand(&cmdHX_);
    ss.addCommand(&cmdHXSingle_);
    ss.addCommand(&cmdNhx_);
    ss.addCommand(&cmdDHT_);
    ss.addCommand(&cmdStepper_);
    ss.addCommand(&cmdServo_);
    ss.addCommand(&cmdPump_);
    ss.addCommand(&cmdStepperAttach_);
    ss.addCommand(&cmdServoAttach_);
    ss.addCommand(&cmdOK_);

    Serial.println("begin");
    LED_OFF();
}

void loop() {
    ss.tick();
    delay(50);
}

void pumpForTime(unsigned long tiempo, uint8_t intensidad)
{
    pump.setPercent(intensidad);
    pump.state(true);

    delay(tiempo);

    pump.state(false);
}
