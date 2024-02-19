// https://github.com/bogde/MultipleHX711

/**
 *
 * MultipleHX711 library for Arduino
 * https://github.com/bogde/MultipleHX711
 *
 * MIT License
 * (c) 2018 Bogdan Necula
 *
**/
#ifndef HX711_REGISTRIES_h
#define HX711_REGISTRIES_h

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// this is only for avr boards, implementation for other archs is needed
#define PORT_B_NUMBER 2 // == digitalPinToPort(8)
#define PORT_C_NUMBER 3 // == digitalPinToPort(A0)
#define PORT_D_NUMBER 4 // == digitalPinToPort(2)

template<size_t N>
class MultipleHX711
{
    private:
        byte PD_SCK;    // Power Down and Serial Clock Input Pin
        byte *DOUT;        // Serial Data Output Pin
        byte GAIN;        // amplification factor

        bool USE_PORT_B = false;
        bool USE_PORT_C = false;
        bool USE_PORT_D = false;

    public:

        MultipleHX711(const byte dout[N], byte pd_sck, byte gain = 128);

        virtual ~MultipleHX711();

        // Initialize library with data output pin, clock input pin and gain factor.
        // Channel selection is made by passing the appropriate gain:
        // - With a gain factor of 64 or 128, channel A is selected
        // - With a gain factor of 32, channel B is selected
        // The library default is "128" (Channel A).
        void begin();

        // Check if MultipleHX711 is ready
        // from the datasheet: When output data is not ready for retrieval, digital output pin DOUT is high. Serial clock
        // input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
        bool isReady();

        // Wait for the MultipleHX711 to become ready
        void waitReady(unsigned long delay_ms = 5);
        bool waitReadyRetry(int retries = 10, unsigned long delay_ms = 5);
        bool waitReadyTimeout(unsigned long timeout = 5000, unsigned long delay_ms = 5);

        // set the gain factor; takes effect only after a call to read()
        // channel A can be set for a 128 or 64 gain; channel B has a fixed 32 gain
        // depending on the parameter, the channel is also set to either A or B
        void setGain(byte gain = 128);

        // waits for the chip to be ready and returns a reading
        bool read(long values[N], unsigned long timeout=0);
        bool readAvg(float values[N], uint8_t n, unsigned long timeout=0);

        // puts the chip into power down mode
        void powerDown();

        // wakes up the chip after power down mode
        void powerUp();
};


/// cosas dificiles ///

template<size_t N>
void readPort(bool states[N], const uint8_t dout[N], bool portB, bool portC, bool portD)
{
    uint8_t valsB=0, valsC=0, valsD=0;
    if (portB)
        valsB = PINB;
    if (portC)
        valsC = PINC;
    if (portD)
        valsD = PIND;

    for (size_t i = 0; i < N; i++)
    {
        uint8_t port = digitalPinToPort(dout[i]);
        uint8_t mask = digitalPinToBitMask(dout[i]);
        if (port == PORT_B_NUMBER)
            states[i] = (bool)(valsB & mask);
        else if (port == PORT_C_NUMBER)
            states[i] = (bool)(valsC & mask);
        else if (port == PORT_D_NUMBER)
            states[i] = (bool)(valsD & mask);
    }
}

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

// TEENSYDUINO has a port of Dean Camera's ATOMIC_BLOCK macros for AVR to ARM Cortex M3.
#define HAS_ATOMIC_BLOCK (defined(ARDUINO_ARCH_AVR) || defined(TEENSYDUINO))

// Whether we are running on either the ESP8266 or the ESP32.
#define ARCH_ESPRESSIF (defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32))

// Whether we are actually running on FreeRTOS.
#define IS_FREE_RTOS defined(ARDUINO_ARCH_ESP32)

// Define macro designating whether we're running on a reasonable
// fast CPU and so should slow down sampling from GPIO.
#define FAST_CPU \
    ( \
    ARCH_ESPRESSIF || \
    defined(ARDUINO_ARCH_SAM)     || defined(ARDUINO_ARCH_SAMD) || \
    defined(ARDUINO_ARCH_STM32)   || defined(TEENSYDUINO) \
    )

#if HAS_ATOMIC_BLOCK
// Acquire AVR-specific ATOMIC_BLOCK(ATOMIC_RESTORESTATE) macro.
#include <util/atomic.h>
#endif

#if FAST_CPU
// Make shiftIn() be aware of clockspeed for
// faster CPUs like ESP32, Teensy 3.x and friends.
// See also:
// - https://github.com/bogde/MultipleHX711/issues/75
// - https://github.com/arduino/Arduino/issues/6561
// - https://community.hiveeyes.org/t/using-bogdans-canonical-hx711-library-on-the-esp32/539

template<size_t N>
void shiftInSlow(uint8_t values[N], const uint8_t dataPin[N], uint8_t clockPin, uint8_t bitOrder, bool portB, bool portC, bool portD) {
    uint8_t i;
    bool states[2];
    memset(values, 0, N*sizeof(uint8_t));

    for(i = 0; i < 8; ++i) {
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(1);

        // read
        readPort<N>(states, dataPin, portB, portC, portD);
        if(bitOrder == LSBFIRST)
        {
            values[0] |= states[0] << i;
            values[1] |= states[1] << i;
        }
        else
        {
            values[0] |= states[0] << (7 - i);
            values[1] |= states[1] << (7 - i);
        }
        digitalWrite(clockPin, LOW);
        delayMicroseconds(1);
    }
}
#else
template<size_t N>
void shiftInSlow(uint8_t values[N], const uint8_t dataPin[N], uint8_t clockPin, uint8_t bitOrder, bool portB, bool portC, bool portD) {
    uint8_t i;
    bool states[2];
    memset(values, 0, N*sizeof(uint8_t));

    for(i = 0; i < 8; ++i) {
        digitalWrite(clockPin, HIGH);

        // read
        readPort<N>(states, dataPin, portB, portC, portD);
        if(bitOrder == LSBFIRST)
        {
            values[0] |= states[0] << i;
            values[1] |= states[1] << i;
        }
        else
        {
            values[0] |= states[0] << (7 - i);
            values[1] |= states[1] << (7 - i);
        }
        digitalWrite(clockPin, LOW);
    }
}
#endif

#if ARCH_ESPRESSIF
// ESP8266 doesn't read values between 0x20000 and 0x30000 when DOUT is pulled up.
#define DOUT_MODE INPUT
#else
#define DOUT_MODE INPUT_PULLUP
#endif

template<size_t N>
MultipleHX711<N>::MultipleHX711(const byte dout[N], byte pd_sck, byte gain)
{
    PD_SCK = pd_sck;
    DOUT = dout;

    for (size_t i = 0; i < N; i++)
    {
        uint8_t port = digitalPinToPort(DOUT[i]);
        if (port == PORT_B_NUMBER) USE_PORT_B = true;
        else if (port == PORT_C_NUMBER) USE_PORT_C = true;
        else if (port == PORT_D_NUMBER) USE_PORT_D = true;
    }

    setGain(gain);
}

template<size_t N>
MultipleHX711<N>::~MultipleHX711() {
}

template<size_t N>
void MultipleHX711<N>::begin() {
    pinMode(PD_SCK, OUTPUT);
    for (size_t i = 0; i < N; i++)
    {
        pinMode(DOUT[i], DOUT_MODE);
    }
}

template<size_t N>
bool MultipleHX711<N>::isReady() {
    // return digitalRead(DOUT) == LOW;
    bool states[2];
    readPort<N>(states, DOUT, USE_PORT_B, USE_PORT_C, USE_PORT_D);
    return !states[0] && !states[1];
}

template<size_t N>
void MultipleHX711<N>::setGain(byte gain) {
    switch (gain) {
        case 128:        // channel A, gain factor 128
            GAIN = 1;
            break;
        case 64:        // channel A, gain factor 64
            GAIN = 3;
            break;
        case 32:        // channel B, gain factor 32
            GAIN = 2;
            break;
    }

}

template<size_t N>
bool MultipleHX711<N>::read(long values[N], unsigned long timeout=0) {

    // Wait for the chip to become ready.
    if (timeout == 0)
        waitReady();
    else
    {
        if (!waitReadyTimeout(timeout))
            return false;
    }

    // Define structures for reading data into.
    unsigned long preValues[N] = {0};
    uint8_t data[3][N] = { 0 };
    uint8_t filler[N] = {0x00};

    // Protect the read sequence from system interrupts.  If an interrupt occurs during
    // the time the PD_SCK signal is high it will stretch the length of the clock pulse.
    // If the total pulse time exceeds 60 uSec this will cause the MultipleHX711 to enter
    // power down mode during the middle of the read sequence.  While the device will
    // wake up when PD_SCK goes low again, the reset starts a new conversion cycle which
    // forces DOUT high until that cycle is completed.
    //
    // The result is that all subsequent bits read by shiftIn() will read back as 1,
    // corrupting the value returned by read().  The ATOMIC_BLOCK macro disables
    // interrupts during the sequence and then restores the interrupt mask to its previous
    // state after the sequence completes, insuring that the entire read-and-gain-set
    // sequence is not interrupted.  The macro has a few minor advantages over bracketing
    // the sequence between `noInterrupts()` and `interrupts()` calls.
    #if HAS_ATOMIC_BLOCK
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

    #elif IS_FREE_RTOS
    // Begin of critical section.
    // Critical sections are used as a valid protection method
    // against simultaneous access in vanilla FreeRTOS.
    // Disable the scheduler and call portDISABLE_INTERRUPTS. This prevents
    // context switches and servicing of ISRs during a critical section.
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    #else
    // Disable interrupts.
    noInterrupts();
    #endif

    // Pulse the clock pin 24 times to read the data.
    shiftInSlow<N>(data[2], DOUT, PD_SCK, MSBFIRST, USE_PORT_B, USE_PORT_C, USE_PORT_D);
    shiftInSlow<N>(data[1], DOUT, PD_SCK, MSBFIRST, USE_PORT_B, USE_PORT_C, USE_PORT_D);
    shiftInSlow<N>(data[0], DOUT, PD_SCK, MSBFIRST, USE_PORT_B, USE_PORT_C, USE_PORT_D);

    // Set the channel and the gain factor for the next reading using the clock pin.
    for (unsigned int i = 0; i < GAIN; i++) {
        digitalWrite(PD_SCK, HIGH);
        #if ARCH_ESPRESSIF
        delayMicroseconds(1);
        #endif
        digitalWrite(PD_SCK, LOW);
        #if ARCH_ESPRESSIF
        delayMicroseconds(1);
        #endif
    }

    #if IS_FREE_RTOS
    // End of critical section.
    portEXIT_CRITICAL(&mux);

    #elif HAS_ATOMIC_BLOCK
    }

    #else
    // Enable interrupts again.
    interrupts();
    #endif

    for (size_t i = 0; i < N; i++)
    {
        // Replicate the most significant bit to pad out a 32-bit signed integer
        if (data[2][i] & 0x80) {
            filler[i] = 0xFF;
        } else {
            filler[i] = 0x00;
        }

        // Construct a 32-bit signed integer
        preValues[i] = ( static_cast<unsigned long>(filler[i]) << 24
                | static_cast<unsigned long>(data[2][i]) << 16
                | static_cast<unsigned long>(data[1][i]) << 8
                | static_cast<unsigned long>(data[0][i]) );

        values[i] = static_cast<long>(preValues[i]);
    }

    return true;
}

template<size_t N>
bool MultipleHX711<N>::readAvg(float values[N], uint8_t n, unsigned long timeout=0)
{
    bool readSuccess;
    long raw[N];
    long sums[N] = {0};
    uint8_t nReads = 0;
    for (uint8_t i = 0; i < n; i++)
    {
        readSuccess = read(raw, timeout);
        if (!readSuccess) continue;

        for (size_t j = 0; j < N; j++)
            sums[j] += raw[j];
        ++nReads;

        #ifdef ARCH_ESPRESSIF
        delay(0);
        #endif
    }

    for (size_t i = 0; i < N; i++)
        values[i] = sums[i] / nReads;

    return (bool)nReads;
}

template<size_t N>
void MultipleHX711<N>::waitReady(unsigned long delay_ms) {
    // Wait for the chip to become ready.
    // This is a blocking implementation and will
    // halt the sketch until a load cell is connected.
    while (!isReady()) {
        // Probably will do no harm on AVR but will feed the Watchdog Timer (WDT) on ESP.
        // https://github.com/bogde/MultipleHX711/issues/73
        delay(delay_ms);
    }
}

template<size_t N>
bool MultipleHX711<N>::waitReadyRetry(int retries, unsigned long delay_ms) {
    // Wait for the chip to become ready by
    // retrying for a specified amount of attempts.
    // https://github.com/bogde/MultipleHX711/issues/76
    int count = 0;
    while (count < retries) {
        if (isReady()) {
            return true;
        }
        delay(delay_ms);
        count++;
    }
    return false;
}

template<size_t N>
bool MultipleHX711<N>::waitReadyTimeout(unsigned long timeout, unsigned long delay_ms) {
    // Wait for the chip to become ready until timeout.
    // https://github.com/bogde/MultipleHX711/pull/96
    unsigned long millisStarted = millis();
    while (millis() - millisStarted < timeout) {
        if (isReady()) {
            return true;
        }
        delay(delay_ms);
    }
    return false;
}

template<size_t N>
void MultipleHX711<N>::powerDown() {
    digitalWrite(PD_SCK, LOW);
    digitalWrite(PD_SCK, HIGH);
}

template<size_t N>
void MultipleHX711<N>::powerUp() {
    digitalWrite(PD_SCK, LOW);
}

#endif /* HX711_REGISTRIES_h */