// https://github.com/adafruit/DHT-sensor-library
#include <DHT.h>
// https://github.com/ppedro74/Arduino-SerialCommands/blob/master/examples/SerialCommandsArguments/SerialCommandsArguments.ino
#include <SerialCommands.h>

#include "Balanzas.h"
#include "PumpManager.h"

#define ARR_LEN(a) sizeof(a)/sizeof(a[0])

// pins
const byte sckPin = 2;
const byte dataPins[] = {5, 6, 7, 8, 9};
const byte dhtPin = 4;
const byte driverPins[4] = {9, 10, 11, 12}; // in1, in2, in3, in4
const byte pumpPin = 3;
const Position pumpPos[] = {{2048, true}, {2048, false}, {4096, true}, {4096, false}};

const size_t nBalanzas = ARR_LEN(dataPins);
const size_t nPosiciones = ARR_LEN(pumpPos);


#define DHT_TYPE DHT22
DHT dht(dhtPin, DHT_TYPE);

#define N_MACETAS nBalanzas

MultipleHX711<nBalanzas, uint8_t> hx711(sckPin, dataPins);

PumpManager<nPosiciones> pump(
    8, 9, 10, 11, // stepper driver pins
    7, // pin servo
    3, // pump pin
    pumpPos, // posiciones
    5000, 15, percent2dutyCycleI(75) // stepper speed (ms per revolution), servo speed (delay in ms between each angle), pump speed (0 = 0%, 256 = 100% of the PWM duty cycle that controls the pump)
);

void cmdUnrecognized(SerialCommands* sender, const char* cmd)
{
    sender->GetSerial()->print("ERROR: No se reconoce el comando \"");
    sender->GetSerial()->print(cmd);
    sender->GetSerial()->println('"');
}

void cmdBalanza(SerialCommands* sender)
{
    // cmd: hx <int:index>
    // respuesta: 12
    // devuelve los datos de la balanza indice

    char* index_str = sender->Next();
    if (index_str == NULL)
    {
        sender->GetSerial()->println("ERROR: No es especifico ningun indice");
        return;
    }

    int index = atoi(index_str);
    if (index < 0 || index >= nBalanzas)
    {
        sender->GetSerial()->print("ERROR: Balanza con indice ");
        sender->GetSerial()->print(index);
        sender->GetSerial()->println(" no existe");
    }

    long value = hx711.read(index);
    if (hx711.error())
        hx711.printError(sender->GetSerial());
    else
        sender->GetSerial()->println(value);
}

void cmdBalanzas(SerialCommands* sender)
{
    // cmd: hxs
    // respuesta: [12,34,56,78,...]
    // devuelve los datos de todas las balanzas

    for (size_t i = 0; i < nBalanzas; i++)
    {
        int value = 0; // completar aca con el valor de la balanza i
        sender->GetSerial()->print('[');
        sender->GetSerial()->print(value);
        if (i < nBalanzas-1)
            sender->GetSerial()->print(',');
        else
            sender->GetSerial()->println(']');
    }
}

void cmdDHT(SerialCommands* sender)
{
    // cmd: dht
    // respuesta: {"hum":12.34,"temp":56.78}
    // devuelve datos del DHT

    float hum = dht.readHumidity();
    float temp = dht.readTemperature();

    sender->GetSerial()->print("{\"hum\":");
    sender->GetSerial()->print(hum);
    sender->GetSerial()->print(",\"temp\":");
    sender->GetSerial()->print(temp);
    sender->GetSerial()->println('}');
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
        sender->GetSerial()->println("ERROR: Los parametros no se especificaron bien");
        return;
    }

    int index = atoi(index_str);
    int tiempo = atoi(tiempo_str); // en ms
    int intensidad = atoi(intensidad_str); // 0-100
    if (index < 0 || index >= N_MACETAS)
    {
        sender->GetSerial()->print("ERROR: El indice de maceta ");
        sender->GetSerial()->print(index);
        sender->GetSerial()->println(" no es aceptable");
        return;
    }
    if (tiempo <= 0)
    {
        sender->GetSerial()->print("ERROR: El tiempo ");
        sender->GetSerial()->print(tiempo);
        sender->GetSerial()->println(" no es aceptable");
        return;
    }
    if (intensidad <= 0 || intensidad > 100)
    {
        sender->GetSerial()->print("ERROR: La intensidad PWM ");
        sender->GetSerial()->print(intensidad);
        sender->GetSerial()->println(" no es aceptable");
        return;
    }

    // regar con esas especificaciones
    uint8_t dutyCycleU8 = percent2dutyCycleI(intensidad);
    bool res = pump.water(index, tiempo, dutyCycleU8);

    if (res)
        sender->GetSerial()->println("OK");
    else
        pump.printError(sender->GetSerial());
}

void cmdStepper(SerialCommands* sender)
{
    // cmd: stepper ?<int:posicion>
    // respuesta: <int:posicion>
    // Devuelve la posicion actual del stepper del sistema de riego
    // Si se transmitio un argumento, este debe ser uint32_t y es el paso al que se debe llevar el stepper

    char* pos_str = sender->Next();
    if (pos_str != NULL)
    {
        // check if is all numbers
        for (size_t i = 0; i < strlen(pos_str); i++)
        {
            if (!isdigit(pos_str[i]) && pos_str[i] != '0') // isdigit returns 0 if argument is not a number or if it is '0'. otherwise it returns > 0
            {
                sender->GetSerial()->print("ERROR: Se recibio el argumento ");
                sender->GetSerial()->print(pos_str);
                sender->GetSerial()->println(" para el comando [stepper], cuando este argumento solo puede ser un numero entero positivo");
                return;
            }
        }

        int newPos = atoi(pos_str);
        pump.stepperGoToPosition(newPos);
    }

    sender->GetSerial()->println(pump.getStepperPos());
}

#define SERIAL_COMMAND_BUFFER_SIZE 32
char serialCommandBuffer[SERIAL_COMMAND_BUFFER_SIZE] = {0};
SerialCommands serialCommands(&Serial, serialCommandBuffer, SERIAL_COMMAND_BUFFER_SIZE, "\n", " ");

SerialCommand cmdBalanza_("hx", cmdBalanza);
SerialCommand cmdBalanzas_("hxs", cmdBalanzas);
SerialCommand cmdDHT_("dht", cmdDHT);
SerialCommand cmdRegar_("water", cmdRegar);
SerialCommand cmdStepper_("stepper", cmdStepper);

void setup()
{
    Serial.begin(9600);

    // dht.begin();

    serialCommands.SetDefaultHandler(cmdUnrecognized);
    serialCommands.AddCommand(&cmdBalanza_);
    serialCommands.AddCommand(&cmdBalanza_);
    serialCommands.AddCommand(&cmdDHT_);
    serialCommands.AddCommand(&cmdRegar_);
    serialCommands.AddCommand(&cmdStepper_);
}

void loop() {
    serialCommands.ReadSerial();
    delay(50);
}
