# Labo7_Labino

## Instalar Arduino-cli

Para instalar la herramienta de comandos de Arduino, correr el siguiente comando

```bash
ARDUINO_DIR="~/Arduino"
if [ ! -d "$ARDUINO_DIR" ]; then
    mkdir ~/Arduino
fi
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=$ARDUINO_DIR sh
chmod a+x $ARDUINO_DIR/arduino-cli
sudo $ARDUINO_DIR/arduino-cli core update-index
sudo $ARDUINO_DIR/arduino-cli core install arduino:avr # esto instala las toolchains para las tarjetas arduino avr
export PATH="$PATH:$ARDUINO_DIR" # esto agrega arduino-cli al path para poder llamarlo con el comando arduino-cli
```

Para poder ejecutar arrduino-cli facilmente en el shell, se agregó al path. Para que este cambio tome efecto, correr el comando ```source ~/.bashrc```

## Librerías

Librerías usadas:
- [PWM - terryjmyers](https://github.com/terryjmyers/PWM)
- [DHT - Adafruit](https://github.com/adafruit/DHT-sensor-library)
- [SerialCommands - ppedro74](https://github.com/ppedro74/Arduino-SerialCommands)

Para instalar librerias .zip de Arduino, instalar ```sudo apt install wget unzip```. Luego

```bash
LIBRARY_ZIP_URL="https://github.com/terryjmyers/PWM/archive/refs/heads/master.zip"
LIBRARY_OUT_FILE="$HOME/pwm.zip"
LIBRARY_NAME="PWM"

if [ ! -d "$HOME/Arduino/libraries/$LIBRARY_NAME" ]; then
    mkdir "$HOME/Arduino/libraries/$LIBRARY_NAME"
fi
wget "$LIBRARY_ZIP_URL" -O "LIBRARY_OUT_FILE"
unzip "LIBRARY_OUT_FILE" -d "$HOME/Arduino/libraries/$LIBRARY_NAME"
rm "LIBRARY_OUT_FILE"
```

Para buscar librerías que están en el library manager de Arduino se debe correr el comando ```arduino-cli lib search <nombre de libraría>```. Luego estas se pueden instalar con el comando ```arduino-cli lib install <nombre de libraría>```. En particular, para este proyecto se debe correr el siguiente comando

```bash
arduino-cli lib install "DHT sensor library"
arduino-cli lib install "SerialCommands"
```

## Compilado

Para usar arduino-cli, hay que declarar explícitamente qué tipo de tarjeta estamos usando (fqbn = fully qualified board name). Algunas fqbn comúnmente usadas son

- ```arduino:avr:uno```
- ```arduino:avr:nano```
- ```arduino:avr:mega```

Para simplificar el proceso podemos declarar una variable en el shell

```bash
FQBN=arduino:avr:nano
```

Para compilar el sketch, correr el siguiente comando en el directorio del proyecto, donde ```.``` es el path al proyecto (si solo se usa ```.``` es porque el directorio actual es el directorio del proyecto)

```bash
arduino-cli compile --fqbn $FQBN .
```

Para encontrar la tarjeta Arduino se puede correr la siguiente linea, que nos dará información, entre otras cosas, de la fqbn de la tajeta conectada, y el puerto serial al que está conectada la tarjeta (por ejemplo ```/dev/ttyUSB0```)

```bash
./arduino-cli board list
```

Ahora podemos cargar el programa compilado a la tarjeta con el siguiente comando, donde ```/dev/ttyUSB0``` es el puerto serial al que está conectada la tarjeta

```bash
arduino-cli upload --fqbn $FQBN -p /dev/ttyUSB0
```

## API

La API del programa consiste en comandos terminados por un caracter de nueva línea ```\n```. Los comandos se envían al puerto serial del Arduino (baudrate de 9600). Los comandos pueden llevar argumentos separados por un caracter de espacio.

|Comando|Argumento 1|Argumento 2|Argumento 3|Respuesta|
|---|---|---|---|---|
|```hx```|```<int:indice>``` (opcional)|-|-|Si se proporciona el argumento indice, devuelve un int con el valor de la balanza correspondiente al indice. Si no se proporcionan argumentos, se devolverá una lista con todos los valores de acda balanza, en orden (```[valor1, valor2, valor3, ...]```)|
|```dht```|-|-|-|Devuelve los datos del DHT en formato JSON (```{"hum":12.34,"temp":56.78}```)|
|```water```|```<int:indice>```|```<int:tiempo>```|```<int:intensidad>```|Riega en la posición correspondiente con el indice, durante el tiempo especificado en tiempo (en milisegundos), con la intensidad de la bomba especificada en intensidad (intensidad va de 1% a 100% de la potencia total). Devuelve el texto "OK"|
|```stepper```|```<int:indice>``` (opcional)|-|-|Si se proporciona el argumento indice, lleva el stepper a la posición correspondiente a la posición del índice nidicado. Con o sin argumentos, devuelve la posición en pasos en que se encuentra el stepper como un número entero|
|```servo```|```<int:angulo>```(opcional)|-|-|Similar a ```stepper```. Si se proporciona un ángulo, lleva el servo a dicho ángulo. En cualquier caso devuelve el ángulo final en el que se encuentra el servo como un número entero|
|```stepper_raw```|```<int:paso>```|-|-|Lleva el stepper al paso indicado en el argumento. Devuelve el paso en el que se encuentra el stepper al final (debería coincidir con el argumento)|


