#ifndef BALANZAS_H
#define BALANZAS_H

#include <Arduino.h>
#include "PROGMEMUtils.h"

#define BALANZAS_ERROR_LOG_STR_MAX_SIZE 128


typedef enum HX711_GAIN : uint8_t
{
    A128,
    A64,
    B32
} HX711_GAIN;

// function definitions
static uint8_t shiftInSlow(const byte dataPin, const byte clockPin, const uint8_t bitOrder, const uint8_t delay_us);
static uint8_t shiftInSlow(const byte dataPin, const byte clockPin, const uint8_t bitOrder);
template <size_t N, class T> static void shiftInSlowMultiple(uint8_t valueBuffer[N], const byte dataPins[N], const byte clockPin, const uint8_t bitOrder, const uint8_t delay_us);
template <size_t N, class T> static void shiftInSlowMultiple(uint8_t valueBuffer[N], const byte dataPins[N], const byte clockPin, const uint8_t bitOrder);
static void pulseSlow(byte clockPin, uint8_t delay_us);
static void pulseSlow(byte clockPin);
static long data2long(const uint8_t data[3]);
static void setGain(const byte sckPin, const HX711_GAIN gain);
////////////

template <size_t N, class T>
class MultipleHX711
{
private:
    byte _sckPin;
    byte _dtPins[N];
    HX711_GAIN _gain;

    char _errorStr[BALANZAS_ERROR_LOG_STR_MAX_SIZE] = {0};
    bool _errorFlag = false;

private:
    bool _indexAllowed(T index, bool showMsg=true)
    {
        if (index < N)
            return true;
        if (showMsg)
        {
            SNPRINTF_FLASH(_errorStr, BALANZAS_ERROR_LOG_STR_MAX_SIZE, F("ERROR: El indice %i es mayor al maximo indice permitido: %i"), index, N);
            _errorFlag = true;
        }
        return false;
    }

public:
    MultipleHX711(byte sckPin, byte dtPins[N], HX711_GAIN gain=A128) : _sckPin(sckPin), _gain(gain)
    {
        memcpy(_dtPins, dtPins, N * sizeof(byte));

        pinMode(_sckPin, OUTPUT);
        for (T i = 0; i < N; i++)
        {
            pinMode(_dtPins[i], INPUT);
        }
    }

    bool isReady(T index)
    {
        if (!_indexAllowed(index)) return false;
        return !digitalRead(_dtPins[index]);
    }

    bool isReadyAll()
    {
        for (T i = 0; i < N; i++)
        {
            if (!isReady(i)) return false;
        }
        return true;
    }

    bool waitReady(T index, unsigned long ms=0)
    {
        if (!_indexAllowed(index))
            return false;
        if (ms == 0)
        {
            while(!isReady(index))
                delay(100);
            return true;
        }
        else
        {
            unsigned long initTime = millis();
            bool ready = isReady(index);
            while(!ready && abs(millis() - initTime) < ms)
            {
                delay(100);
                ready = isReady(index);
            }
            if (!ready)
            {
                SNPRINTF_FLASH(_errorStr, BALANZAS_ERROR_LOG_STR_MAX_SIZE, F("ERROR: La balanza de indice %i no esta lista y se termino el tiempo de espera"), index);
                _errorFlag = true;
            }
            return ready;
        }
    }

    bool waitReadyAll(unsigned long ms=0)
    {
        if (ms == 0)
        {
            while(!isReadyAll())
                delay(100);
            return true;
        }
        else
        {
            unsigned long initTime = millis();
            bool ready = isReadyAll();
            while(!ready && abs(millis() - initTime) < ms)
            {
                delay(100);
                ready = isReadyAll();
            }
            if (!ready)
            {
                SNPRINTF_FLASH_NO_ARGS(_errorStr, BALANZAS_ERROR_LOG_STR_MAX_SIZE, F("ERROR: No todas las balanzas estan listas y se termino el tiempo de espera"));
                _errorFlag = true;
            }
            return ready;
        }
    }

    long read(T index, unsigned long ms=0)
    {
        //https://github.com/bogde/HX711/blob/master/src/HX711.cpp
        // Wait for the chip to become ready.
        if (!waitReady(index, ms))
        {
            _errorFlag = true;
            return 0;
        }

        // Define structures for reading data into.
        uint8_t data[3] = { 0 };

        // Protect the read sequence from system interrupts.  If an interrupt occurs during
        // the time the PD_SCK signal is high it will stretch the length of the clock pulse.
        // If the total pulse time exceeds 60 uSec this will cause the HX711 to enter
        // power down mode during the middle of the read sequence.  While the device will
        // wake up when PD_SCK goes low again, the reset starts a new conversion cycle which
        // forces DOUT high until that cycle is completed.
        
        // The result is that all subsequent bits read by shiftIn() will read back as 1,
        // corrupting the value returned by read().  The ATOMIC_BLOCK macro disables
        // interrupts during the sequence and then restores the interrupt mask to its previous
        // state after the sequence completes, insuring that the entire read-and-gain-set
        // sequence is not interrupted.  The macro has a few minor advantages over bracketing
        // the sequence between `noInterrupts()` and `interrupts()` calls.
        #if HAS_ATOMIC_BLOCK
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        #else
        // Disable interrupts.
        noInterrupts();
        #endif

        data[2] = shiftInSlow(_dtPins[index], _sckPin, MSBFIRST);
        data[1] = shiftInSlow(_dtPins[index], _sckPin, MSBFIRST);
        data[0] = shiftInSlow(_dtPins[index], _sckPin, MSBFIRST);

        // Set the channel and the gain factor for the next reading using the clock pin.
        setGain(_sckPin, _gain);

        #if HAS_ATOMIC_BLOCK
        }
        #else
        // Enable interrupts again.
        interrupts();
        #endif

        return data2long(data);
    }
    bool readAll(long valuesBuffer[N], unsigned long ms=0)
    {
        uint8_t data[3][N] = { 0 };

        // Wait for the chip to become ready.
        if (!waitReadyAll(ms))
        {
            _errorFlag = true;
            return 0;   
        }

        #if HAS_ATOMIC_BLOCK
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        #else
        // Disable interrupts.
        noInterrupts();
        #endif

        shiftInSlowMultiple<N, T>(data[0], _dtPins, _sckPin, MSBFIRST);
        shiftInSlowMultiple<N, T>(data[1], _dtPins, _sckPin, MSBFIRST);
        shiftInSlowMultiple<N, T>(data[2], _dtPins, _sckPin, MSBFIRST);

        #if HAS_ATOMIC_BLOCK
        }
        #else
        // Enable interrupts again.
        interrupts();
        #endif

        setGain(_sckPin, _gain);

        for (T i = 0; i < N; i++)
        {
            const uint8_t dataTemp[3] = {data[0][i], data[1][i], data[2][i]};
            valuesBuffer[i] = data2long(dataTemp);
        }

        return true;
    }

    float readAvg(T index, size_t n=20, unsigned long ms=0)
    {
        // https://stackoverflow.com/questions/28820904/how-to-efficiently-compute-average-on-the-fly-moving-average
        float avg = 0, a, b;
        for (size_t m = 1; m < n+1; m++)
        {
            float newVal = static_cast<float>(read(index, ms));
            a = 1 / m;
            b = 1 - a;
            avg = a * newVal + b * avg;
        }
        return avg;
    }

    bool readAllAvg(float *valuesBuffer, size_t valuesBufferLen, size_t n=20, unsigned long ms=0)
    {
        // https://stackoverflow.com/questions/28820904/how-to-efficiently-compute-average-on-the-fly-moving-average

        float a, b;
        bool readSuccess;
        long buff[N] = {0};
        for (size_t m = 1; m < n+1; m++)
        {
            a = 1 / m;
            b = 1 - a;
            readSuccess = readAll(buff, N);
            if (!readSuccess) continue;
            for (T i = 0; i < N; i++)
            {
                float newVal = static_cast<float>(buff[i]);
                valuesBuffer[i] = a * newVal + b * valuesBuffer[i];
            }
        }

        return true;
    }

    void powerDown() const
    {
        digitalWrite(_sckPin, LOW);
        delayMicroseconds(5);
        digitalWrite(_sckPin, HIGH);
        delayMicroseconds(5);
    }
    void powerUp() const
    {
        digitalWrite(_sckPin, LOW);
        delayMicroseconds(5);
        for (uint8_t i = 0; i < 24; i++)
        {
            pulseSlow(_sckPin);
        }
        setGain(_sckPin, _gain);
    }

    inline bool error() const { return _errorFlag; }
    inline void printError(Stream *stream)
    {
        stream->println(_errorStr);
        _errorFlag = false;
        memset(_errorStr, '\0', BALANZAS_ERROR_LOG_STR_MAX_SIZE);
    }
};

////// COSAS DIFICILES ////////

#if HAS_ATOMIC_BLOCK
// Acquire AVR-specific ATOMIC_BLOCK(ATOMIC_RESTORESTATE) macro.
#include <util/atomic.h>
#endif


# ifdef SHIFTIN_SLOW
static uint8_t shiftInSlow(const byte dataPin, const byte clockPin, const uint8_t bitOrder, const uint8_t delay_us=2) {
    uint8_t value = 0;

    for(uint8_t i = 0; i < 8; ++i) {
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(delay_us);
        if(bitOrder == LSBFIRST)
            value |= digitalRead(dataPin) << i;
        else
            value |= digitalRead(dataPin) << (7 - i);
        digitalWrite(clockPin, LOW);
        delayMicroseconds(delay_us);
    }
    return value;
}
template <size_t N, class T>
static void shiftInSlowMultiple(uint8_t valueBuffer[N], const byte dataPins[N], const byte clockPin, const uint8_t bitOrder, const uint8_t delay_us=2)
{
    // valueBuffer HAS TO BE THE SAME LENGTH AS dataPins (or longer)
    uint8_t values[N] = {0};

    for(uint8_t i = 0; i < 8; ++i)
    {
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(delay_us);
        for (T j = 0; j < N; j++)
        {
            if(bitOrder == LSBFIRST)
                valueBuffer[j] |= digitalRead(dataPin) << i;
            else
                valueBuffer[j] |= digitalRead(dataPin) << (7 - i);
        }
        digitalWrite(clockPin, LOW);
        delayMicroseconds(delay_us);
    }
}
static void pulseSlow(byte clockPin, uint8_t delay_us=2)
{
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(delay_us);
    digitalWrite(clockPin, LOW);
    delayMicroseconds(delay_us);
}
// #define SHIFTIN_WITH_SPEED_SUPPORT(data,clock,order,delay_us) shiftInSlow(data,clock,order,delay_us)
// #define SHIFTIN_WITH_SPEED_SUPPORT(data,clock,order) shiftInSlow(data,clock,order)
// #define SHIFTIN_MULTIPLE_WITH_SPEED_SUPPORT(N,T,buffer,datas,clock,order,delay_us) shiftInSlowMultiple<N, T>(buffer,datas,clock,order,delay_us)
// #define SHIFTIN_MULTIPLE_WITH_SPEED_SUPPORT(N,T,buffer,datas,clock,order) shiftInSlowMultiple<N, T>(buffer,datas,clock,order)
// #define PULSE_WITH_SPEED_SUPPORT(clock,delay_us) pulseSlow(clock,delay_us)
// #define PULSE_WITH_SPEED_SUPPORT(clock) pulseSlow(clock)
#else
static uint8_t shiftInSlow(const byte dataPin, const byte clockPin, const uint8_t bitOrder, const uint8_t delay_us) {
    return shiftIn(dataPin, clockPin, bitOrder);
}
static uint8_t shiftInSlow(const byte dataPin, const byte clockPin, const uint8_t bitOrder) {
    return shiftIn(dataPin, clockPin, bitOrder);
}

template <size_t N, class T>
static void shiftInSlowMultiple(uint8_t valueBuffer[N], const byte dataPins[N], const byte clockPin, const uint8_t bitOrder)
{
    // valueBuffer HAS TO BE THE SAME LENGTH AS dataPins (or longer)
    uint8_t values[N] = {0};

    for(uint8_t i = 0; i < 8; ++i)
    {
        digitalWrite(clockPin, HIGH);
        for (T j = 0; j < N; j++)
        {
            if(bitOrder == LSBFIRST)
                valueBuffer[j] |= digitalRead(dataPins[j]) << i;
            else
                valueBuffer[j] |= digitalRead(dataPins[j]) << (7 - i);
        }
        digitalWrite(clockPin, LOW);
    }
}
static void pulseSlow(byte clockPin)
{
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
}
static void pulseSlow(byte clockPin, uint8_t delay_us)
{
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
}
// #define SHIFTIN_WITH_SPEED_SUPPORT(data,clock,order,delay_us) shiftIn(data,clock,order)
// #define SHIFTIN_WITH_SPEED_SUPPORT(data,clock,order) shiftIn(data,clock,order)
// #define SHIFTIN_MULTIPLE_WITH_SPEED_SUPPORT(buffer,datas,nData,clock,order,delay_us) shiftInMultiple(buffer,datas,nData,clock,order)
// #define SHIFTIN_MULTIPLE_WITH_SPEED_SUPPORT(buffer,datas,nData,clock,order) shiftInMultiple(buffer,datas,nData,clock,order)
// #define PULSE_WITH_SPEED_SUPPORT(clock,delay_us) pulseFast(clock)
// #define PULSE_WITH_SPEED_SUPPORT(clock) pulseFast(clock)
#endif

static long data2long(const uint8_t data[3])
{
    unsigned long value = 0;
    uint8_t filler = 0x00;

    // Replicate the most significant bit to pad out a 32-bit signed integer
	if (data[2] & 0x80) {
		filler = 0xFF;
	} else {
		filler = 0x00;
	}

	// Construct a 32-bit signed integer
	value = ( static_cast<unsigned long>(filler) << 24
			| static_cast<unsigned long>(data[2]) << 16
			| static_cast<unsigned long>(data[1]) << 8
			| static_cast<unsigned long>(data[0]) );

	return static_cast<long>(value);
}

static void setGain(const byte sckPin, const HX711_GAIN gain)
{
    // Set the channel and the gain factor for the next reading using the clock pin.
    switch(gain)
    {
        case A64:
            pulseSlow(sckPin);
        case B32:
            pulseSlow(sckPin);
        case A128:
            pulseSlow(sckPin);
            break;  
        default:
            pulseSlow(sckPin);
            pulseSlow(sckPin);
            pulseSlow(sckPin);
            break;
    }
}

#endif