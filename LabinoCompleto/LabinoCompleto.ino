// https://github.com/adafruit/DHT-sensor-library
#include <DHT.h>

#include "SmartSerial.h"
#include "Balanzas.h"
#include "MovementManager.h"
#include "PWMHelper.h"

#define ARR_LEN(a) sizeof(a)/sizeof(a[0])

// pins
const byte sckPin = 2;
const byte dataPins[] = {5, A4};
const byte dhtPin = 4;
const byte driverPins[4] = {8, 9, 10, 11}; // in1, in2, in3, in4
const byte servoPin = 7;
const byte pumpPin = 3;
const Position pumpPos[] = {{1100, true}, {1100, false}, {2200, true}, {2200, false}};

const size_t nBalanzas = ARR_LEN(dataPins);
const size_t nPosiciones = ARR_LEN(pumpPos);

#define DHT_TYPE DHT22
DHT dht(dhtPin, DHT_TYPE);

MultipleHX711<nBalanzas> hx711(dataPins, sckPin);

MovementManager<nPosiciones> movement(
    driverPins[0], driverPins[1], driverPins[2], driverPins[3], // stepper driver pins
    servoPin, // pin servo
    pumpPos, // posiciones
    1000, 5  // stepper speed (ms per revolution), servo speed (delay in ms between each angle), pump speed (0 = 0%, 256 = 100% of the PWM duty cycle that controls the pump)
);

PWMPin pump(pumpPin, percent2dutyCycleI(50));

bool water(Stream *stream, size_t index, unsigned long tiempo, uint8_t intensidad, bool returnHome=true);

#define LED_ON() digitalWrite(LED_BUILTIN, HIGH)
#define LED_OFF() digitalWrite(LED_BUILTIN, LOW)

void cmdUnrecognized(Stream *stream, const char* cmd)
{
    LED_ON();
    stream->print(F("ERROR: No se reconoce el comando \""));
    stream->print(cmd);
    stream->println(F("\""));
    LED_OFF();
}

void cmdBalanza(Stream *stream, CommandArguments *comArgs)
{
    // cmd: hx
    // respuesta: [12,34,56,78,...]
    // devuelve los datos de la balanza

    // devuelve todas las balanzas

    LED_ON();
    long values[nBalanzas];
    bool s = hx711.read(values);
    if (!s)
    {
        stream->println(F("ERROR: No se pudo leer las balanzas"));
        LED_OFF();
        return;
    }
    for (size_t i = 0; i < nBalanzas; i++)
    {
        stream->print('[');
        stream->print(values[i]);
        if (i < nBalanzas-1)
            stream->print(',');
        else
            stream->println(']');
    }
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

void cmdRegar(Stream *stream, CommandArguments *comArgs)
{
    // cmd: water <int:index> <int:tiempo> <int:intensidad>
    // respuesta: ok
    // riega en la posicion <index>, la cantidad de milisegundos <tiempo>, con una intensidad de pwm <intensidad>

    LED_ON();
    if (comArgs->N != 3)
    {
        stream->println(F("ERROR: Los argumentos no se especificaron bien"));
        LED_OFF();
        return;
    }

    long int index, tiempo, intensidad;
    bool isIntIndex = comArgs->toInt(0, &index);
    bool isIntTiempo = comArgs->toInt(1, &tiempo); // en ms
    bool isIntIntensidad = comArgs->toInt(2, &intensidad); // 0-100
    if (!(isIntIndex && isIntTiempo && isIntIntensidad))
    {
        stream->print(F("ERROR: Alguno de los argumentos no es un numero entero. Los argumentos recibidos son, en orden, "));
        stream->print(index);
        stream->print(F(", "));
        stream->print(tiempo);
        stream->print(F(", "));
        stream->println(intensidad);
        LED_OFF();
        return;
    }

    // regar con esas especificaciones
    bool res = water(stream, index, tiempo, intensidad);

    if (res)
        stream->println(F("OK"));

    LED_OFF();
}

void cmdStepper(Stream *stream, CommandArguments *comArgs)
{
    // cmd: stepper ?<int:index>
    // respuesta: <int:posicion>
    // Devuelve la posicion actual del stepper del sistema de riego
    // Si se transmitio un argumento, este debe ser el indice de la posicion a la que se quiere llevar el stepper

    LED_ON();
    if (comArgs->N == 1)
    {
        // check if it is a number
        long int index;
        bool isInt = comArgs->toInt(0, &index);

        if (!isInt)
        {
            stream->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
            stream->println(comArgs->arg(0));
            LED_OFF();
            return;
        }
        else if (index < 0 || index > nPosiciones)
        {
            stream->print(F("ERROR: El argumento es un numero menor a 0 o mayor a "));
            stream->print(nPosiciones-1);
            stream->print(F(". El numero del argumento es "));
            stream->println(index);
            LED_OFF();
            return;
        }

        bool success = movement.stepperGoToPosition(index);

        if (!success)
        {
            movement.printError(stream);
            LED_OFF();
            return;
        }
    }

    stream->println(movement.getStepperStep());
    LED_OFF();
}

void cmdServo(Stream *stream, CommandArguments *comArgs)
{
    // cmd: servo ?<int:angulo>
    // respuesta: <int:angulo>
    // si se especifica un angulo, se movera el servo hasta ese angulo. en cualquier
    // caso, devuelve el angulo final del servo

    LED_ON();
    if (comArgs->N == 1)
    {
        // check if is all numbers
        long int angle;
        bool isInt = comArgs->toInt(0, &angle);
        if (!isInt)
        {
            stream->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
            stream->println(comArgs->arg(0));
            LED_OFF();
            return;
        }
        else if (angle <= SERVO_MIN_ANGLE && angle >= SERVO_MAX_ANGLE)
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

        movement.servoGoToAngle(angle);
    }

    stream->println(movement.getServoAngle());
    LED_OFF();
}

void cmdPos(Stream *stream, CommandArguments *comArgs)
{
    // cmd: pos <int:index || std:home>
    // respuesta: OK
    // posiciona el sistema en la posicion del indice indicado

    LED_ON();
    if (comArgs->N == 0)
    {
        stream->println(F("ERROR: No se proporcino un argumento numerico."));
        LED_OFF();
        return;
    }

    bool success;
    if (strcmp(comArgs->arg(0), "home") == 0)
    {
        bool s1 = movement.servoGoHome();
        bool s2 = movement.stepperGoHome();
        success = s1 && s2;
    }
    else
    {
        // check if is a number
        long int index;
        bool isInt = comArgs->toInt(0, &index);

        if (!isInt)
        {
            stream->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
            stream->println(comArgs->arg(0));
            LED_OFF();
            return;
        }

        if (index < -1 || index >= nPosiciones)
        {
            stream->print(F("ERROR: El argumento deberia ser un indice entre 0 y "));
            stream->print(nPosiciones-1);
            stream->print(F(" o -1 (posicion home) pero es el numero "));
            stream->println(index);
            LED_OFF();
            return;
        }

        bool s1 = movement.stepperGoToPosition(index);
        bool s2 = movement.servoGoToPosition(index);
        success = s1 && s2;
    }

    if (!success)
        movement.printError(stream);
    else
        stream->println(F("OK"));
    LED_OFF();
}

void cmdStepperRaw(Stream *stream, CommandArguments *comArgs)
{
    // cmd: stepper_raw <int:index>
    // respuesta: <int:posicion>
    // Devuelve la posicion actual del stepper del sistema de riego
    // Si se transmitio un argumento, este debe ser uint32_t y es el paso al que se debe llevar el stepper

    LED_ON();
    if (comArgs->N == 0)
    {
        stream->println(F("ERROR: No se proporcino un argumento numerico."));
        LED_OFF();
        return;
    }

    // check if is a number
    long int step;
    bool isInt = comArgs->toInt(0, &step);

    if (!isInt)
    {
        stream->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
        stream->println(comArgs->arg(0));
        LED_OFF();
        return;
    }

    bool success = movement.stepperGoToStep(step);

    if (!success)
    {
        movement.printError(stream);
        LED_OFF();
        return;
    }

    stream->println(movement.getStepperStep());
    LED_OFF();
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

    movement.stepperAttach(attach);

    stream->println(attach ? F("1") : F("0"));
    LED_OFF();
}

void cmdHello(Stream *stream, CommandArguments *comArgs)
{
    // cmd: ok
    // respuesta: OK

    LED_ON();
    stream->println(F("OK"));
    delay(500);
    LED_OFF();
}

SmartSerial ss(&Serial);

SmartCommand cmdBalanza_("hx", cmdBalanza);
SmartCommand cmdDHT_("dht", cmdDHT);
SmartCommand cmdRegar_("water", cmdRegar);
SmartCommand cmdStepper_("stepper", cmdStepper);
SmartCommand cmdServo_("servo", cmdServo);
SmartCommand cmdPos_("pos", cmdPos);
SmartCommand cmdStepperRaw_("stepper_raw", cmdStepperRaw);
SmartCommand cmdStepperAttach_("stepper_attach", cmdStepperAttach);
SmartCommand cmdHello_("ok", cmdHello);

void setup()
{
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    movement.begin();
    hx711.begin();

    dht.begin();

    ss.setDefaultCallback(cmdUnrecognized);
    ss.addCommand(&cmdBalanza_);
    ss.addCommand(&cmdDHT_);
    ss.addCommand(&cmdRegar_);
    ss.addCommand(&cmdStepper_);
    ss.addCommand(&cmdServo_);
    ss.addCommand(&cmdPos_);
    ss.addCommand(&cmdStepperRaw_);
    ss.addCommand(&cmdStepperAttach_);
    ss.addCommand(&cmdHello_);
}

void loop() {
    ss.tick();
    delay(50);
}


bool water(Stream *stream, size_t index, unsigned long tiempo, uint8_t intensidad, bool returnHome=true)
{
    if (index >= nPosiciones)
    {
        stream->print(F("ERROR: El indice de maceta recibido es "));
        stream->print(index);
        stream->print(F(". Debe ser mayor o igual a 0 y menor que "));
        stream->print(nBalanzas);
        return false;
    }
    else if (tiempo <= 0)
    {
        stream->print(F("ERROR: El tiempo recibido es "));
        stream->print(tiempo);
        stream->println(F(". Debe ser mayor a 0"));
        return false;
    }
    else if (intensidad <= 0 || intensidad > 100)
    {
        stream->print(F("ERROR: La intensidad PWM recibida es "));
        stream->print(intensidad);
        stream->println(F(". Debe ser mayor a 0 y menor o igual a 100"));
        return false;
    }

    bool success = movement.goToPosition(index, true, false);

    if (!success)
    {
        movement.printError(stream);
        return false;
    }

    pump.setPercent(intensidad);
    pump.state(true);

    delay(tiempo);

    pump.state(false);

    if (returnHome)
    {
        success = movement.goHome(false, true);
    }

    if (!success)
    {
        movement.printError(stream);
        return false;
    }

    return true;
}
