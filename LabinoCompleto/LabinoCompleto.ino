// https://github.com/adafruit/DHT-sensor-library
#include <DHT.h>
// https://github.com/ppedro74/Arduino-SerialCommands/blob/master/examples/SerialCommandsArguments/SerialCommandsArguments.ino
#include <SerialCommands.h>

#include "Balanzas.h"
#include "PumpManager.h"

#define ARR_LEN(a) sizeof(a)/sizeof(a[0])

// pins
const byte sckPin = 2;
const byte dataPins[] = {2, 5, 6, 12, 13};
const byte dhtPin = 4;
const byte driverPins[4] = {8, 9, 10, 11}; // in1, in2, in3, in4
const byte servoPin = 7;
const byte pumpPin = 3;
const Position pumpPos[] = {{2048, true}, {2048, false}, {4096, true}, {4096, false}};

const size_t nBalanzas = ARR_LEN(dataPins);
const size_t nPosiciones = ARR_LEN(pumpPos);


#define DHT_TYPE DHT22
DHT dht(dhtPin, DHT_TYPE);

MultipleHX711<nBalanzas, uint8_t> hx711(sckPin, dataPins);

PumpManager<nPosiciones> pump(
    8, 9, 10, 11, // stepper driver pins
    7, // pin servo
    3, // pump pin
    pumpPos, // posiciones
    5000, 15, percent2dutyCycleI(75) // stepper speed (ms per revolution), servo speed (delay in ms between each angle), pump speed (0 = 0%, 256 = 100% of the PWM duty cycle that controls the pump)
);

bool smartAtoi(long int *i, const char *charr)
{
    char *endptr;
    *i = strtol(charr, &endptr, 10);
    return charr[0] != '\0' && *endptr == '\0';
}

void cmdUnrecognized(SerialCommands* sender, const char* cmd)
{
    sender->GetSerial()->print(F("ERROR: No se reconoce el comando \""));
    sender->GetSerial()->print(cmd);
    sender->GetSerial()->println(F("\""));
}

void cmdBalanza(SerialCommands* sender)
{
    // cmd: hx ?<int:index>
    // respuesta: 12 (si hay index); [12,34,56,78,...] (si no hay index)
    // devuelve los datos de la balanza

    char* index_str = sender->Next();
    if (index_str == NULL)
    {
        // devuelve todas las balanzas
        long values[nBalanzas];
        hx711.readAll(values);
        if (hx711.error())
        {
            hx711.printError(sender->GetSerial());
            return;
        }
        for (size_t i = 0; i < nBalanzas; i++)
        {
            sender->GetSerial()->print('[');
            sender->GetSerial()->print(values[i]);
            if (i < nBalanzas-1)
                sender->GetSerial()->print(',');
            else
                sender->GetSerial()->println(']');
        }
        return;
    }

    long int index;
    bool isInt = smartAtoi(&index, index_str);
    if (!isInt)
    {
        sender->GetSerial()->print(F("ERROR: El argumento no es un entero. El argumento provisto es "));
        sender->GetSerial()->println(index_str);
        return;
    }
    else if (index < 0 || index >= nBalanzas)
    {
        sender->GetSerial()->print(F("ERROR: Balanza con indice "));
        sender->GetSerial()->print(index);
        sender->GetSerial()->print(F(" no existe. El indice debe estar entre 0 y "));
        sender->GetSerial()->println(nBalanzas);
        return;
    }

    long value = hx711.read(index);
    if (hx711.error())
        hx711.printError(sender->GetSerial());
    else
        sender->GetSerial()->println(value);
}

void cmdDHT(SerialCommands* sender)
{
    // cmd: dht
    // respuesta: {"hum":12.34,"temp":56.78}
    // devuelve datos del DHT

    float hum = dht.readHumidity();
    float temp = dht.readTemperature();

    sender->GetSerial()->print(F("{\"hum\":"));
    sender->GetSerial()->print(hum);
    sender->GetSerial()->print(F(",\"temp\":"));
    sender->GetSerial()->print(temp);
    sender->GetSerial()->println(F("}"));
}

void cmdRegar(SerialCommands* sender)
{
    // cmd: water <int:index> <int:tiempo> <int:intensidad>
    // respuesta: ok
    // riega en la posicion <index>, la cantidad de milisegundos <tiempo>, con una intensidad de pwm <intensidad>

    char* index_str = sender->Next();
    char* tiempo_str = sender->Next();
    char* intensidad_str = sender->Next();
    if (index_str == NULL || tiempo_str == NULL || intensidad_str == NULL)
    {
        sender->GetSerial()->println(F("ERROR: Los argumentos no se especificaron bien"));
        return;
    }

    long int index, tiempo, intensidad;
    bool isIntIndex = smartAtoi(&index, index_str);
    bool isIntTiempo = smartAtoi(&index, tiempo_str); // en ms
    bool isIntIntensidad = smartAtoi(&index, intensidad_str); // 0-100
    if (!(isIntIndex && isIntTiempo && isIntIntensidad))
    {
        sender->GetSerial()->print(F("ERROR: Alguno de los argumentos no es un numero entero. Los argumentos recibidos son, en orden, "));
        sender->GetSerial()->print(index);
        sender->GetSerial()->print(F(", "));
        sender->GetSerial()->print(tiempo);
        sender->GetSerial()->print(F(", "));
        sender->GetSerial()->println(intensidad);
        return;
    }

    if (index < 0 || index >= nBalanzas)
    {
        sender->GetSerial()->print(F("ERROR: El indice de maceta recibido es "));
        sender->GetSerial()->print(index);
        sender->GetSerial()->print(F(". Debe ser mayor o igual a 0 y menor que "));
        sender->GetSerial()->print(nBalanzas);
        return;
    }
    if (tiempo <= 0)
    {
        sender->GetSerial()->print(F("ERROR: El tiempo recibido es "));
        sender->GetSerial()->print(tiempo);
        sender->GetSerial()->println(F(". Debe ser mayor a 0"));
        return;
    }
    if (intensidad <= 0 || intensidad > 100)
    {
        sender->GetSerial()->print(F("ERROR: La intensidad PWM recibida es "));
        sender->GetSerial()->print(intensidad);
        sender->GetSerial()->println(F(". Debe ser mayor a 0 y menor o igual a 100"));
        return;
    }

    // regar con esas especificaciones
    uint8_t dutyCycleU8 = percent2dutyCycleI(intensidad);
    bool res = pump.water(index, tiempo, dutyCycleU8);

    if (res)
        sender->GetSerial()->println(F("OK"));
    else
        pump.printError(sender->GetSerial());
}

void cmdStepper(SerialCommands* sender)
{
    // cmd: stepper ?<int:index>
    // respuesta: <int:posicion>
    // Devuelve la posicion actual del stepper del sistema de riego
    // Si se transmitio un argumento, este debe ser el indice de la posicion a la que se quiere llevar el stepper

    char* index_str = sender->Next();
    if (index_str != NULL)
    {
        // check if is all numbers
        long int index;
        bool isInt = smartAtoi(&index, index_str);

        if (!isInt)
        {
            sender->GetSerial()->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
            sender->GetSerial()->println(index_str);
            return;
        }
        else if (index < 0 || index > nPosiciones)
        {
            sender->GetSerial()->print(F("ERROR: El argumento es un numero menor a 0 o mayor a "));
            sender->GetSerial()->print(nPosiciones);
            sender->GetSerial()->print(F(". El numero del argumento es "));
            sender->GetSerial()->println(index);
            return;
        }

        bool success = pump.stepperGoToPosition(index);

        if (!success)
        {
            pump.printError(sender->GetSerial());
            return;
        }
    }

    sender->GetSerial()->println(pump.getStepperStep());
}

void cmdServo(SerialCommands* sender)
{
    // cmd: servo ?<int:angulo>
    // respuesta: <int:angulo>
    // si se especifica un angulo, se movera el servo hasta ese angulo. en cualquier
    // caso, devuelve el angulo final del servo

    char* ang_str = sender->Next();
    if (ang_str != NULL)
    {
        // check if is all numbers
        long int angle;
        bool isInt = smartAtoi(&angle, ang_str);
        if (!isInt)
        {
            sender->GetSerial()->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
            sender->GetSerial()->println(ang_str);
            return;
        }
        else if (angle >= SERVO_MIN_ANGLE && angle <= SERVO_MAX_ANGLE)
        {
            sender->GetSerial()->print(F("ERROR: El argumento es un numero menor a "));
            sender->GetSerial()->print(SERVO_MIN_ANGLE);
            sender->GetSerial()->print(F(" o mayor a "));
            sender->GetSerial()->print(SERVO_MAX_ANGLE);
            sender->GetSerial()->print(F(". El numero del argumento es "));
            sender->GetSerial()->println(angle);
            return;
        }

        pump.servoGoToAngle(angle);
    }

    sender->GetSerial()->println(pump.getServoAngle());
}

void cmdStepperRaw(SerialCommands* sender)
{
    // cmd: stepper_raw <int:index>
    // respuesta: <int:posicion>
    // Devuelve la posicion actual del stepper del sistema de riego
    // Si se transmitio un argumento, este debe ser uint32_t y es el paso al que se debe llevar el stepper

    char* step_str = sender->Next();
    if (step_str != NULL)
    {
        sender->GetSerial()->println(F("ERROR: No se proporcino un argumento numerico."));
        return;
    }

    // check if is all numbers
    long int step;
    bool isInt = smartAtoi(&step, step_str);

    if (!isInt)
    {
        sender->GetSerial()->print(F("ERROR: El argumento no es un numero entero. El argumento es "));
        sender->GetSerial()->println(step_str);
        return;
    }

    bool success = pump.stepperGoToStep(step);

    if (!success)
    {
        pump.printError(sender->GetSerial());
        return;
    }

    sender->GetSerial()->println(pump.getStepperStep());
}

void cmdHello(SerialCommands* sender)
{
    // cmd: hello
    // respuesta: OK
    sender->GetSerial()->println(F("OK"));
}

#define SERIAL_COMMAND_BUFFER_SIZE 32
char serialCommandBuffer[SERIAL_COMMAND_BUFFER_SIZE] = {0};
SerialCommands serialCommands(&Serial, serialCommandBuffer, SERIAL_COMMAND_BUFFER_SIZE, "\n", " ");

SerialCommand cmdBalanza_("hx", cmdBalanza);
SerialCommand cmdDHT_("dht", cmdDHT);
SerialCommand cmdRegar_("water", cmdRegar);
SerialCommand cmdStepper_("stepper", cmdStepper);
SerialCommand cmdServo_("servo", cmdServo);
SerialCommand cmdStepperRaw_("stepper_raw", cmdStepperRaw);
SerialCommand cmdHello_("hello", cmdHello);

void setup()
{
    Serial.begin(9600);

    dht.begin();

    serialCommands.SetDefaultHandler(cmdUnrecognized);
    serialCommands.AddCommand(&cmdBalanza_);
    serialCommands.AddCommand(&cmdDHT_);
    serialCommands.AddCommand(&cmdRegar_);
    serialCommands.AddCommand(&cmdStepper_);
    serialCommands.AddCommand(&cmdServo_);
    serialCommands.AddCommand(&cmdStepperRaw_);
    serialCommands.AddCommand(&cmdHello_);
}

void loop() {
    serialCommands.ReadSerial();
    delay(50);
}
