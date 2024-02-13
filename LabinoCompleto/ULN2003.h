#ifndef ULN_2003_H
#define ULN_2003_H

// https://programarfacil.com/blog/arduino-blog/motor-paso-a-paso-uln2003-l298n/

#include <Arduino.h>
#include "PROGMEMUtils.h"

#define STEPS_PER_REVOLUTION 32 // 64 half steps
#define STEPS_PER_REVOLUTION_WITH_GEARING 2048 // 4096 half steps
#define MIN_US_PER_STEP 1000//750
#define ULN_2003_ERROR_LOG_STR_MAX_SIZE 256

static bool stepWave [4][4] =
{
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
};

static bool stepNormal [4][4] =
{
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1}
};

static bool stepHalf [8][4] =
{
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

typedef enum MovementType : uint8_t {
    HALF,
    NORMAL,
    WAVE
} MovementType;

class ULN2003
{
private:
    int32_t _currPosition = 0; // how many steps have been taken and in which direction
    MovementType _movementType;
    uint32_t _usPerStep = MIN_US_PER_STEP;
    byte _in1, _in2, _in3, _in4;
    bool _attached = false;

    bool _errorFlag = false;
    char _errorStr[ULN_2003_ERROR_LOG_STR_MAX_SIZE] = {0};

    
    inline void makeStepHalfClockwise(bool direction=true) const
    {
        for (int8_t i = 0; i < 8; i++)
        {
            digitalWrite(_in1, stepHalf[i][0]);
            digitalWrite(_in2, stepHalf[i][1]);
            digitalWrite(_in3, stepHalf[i][2]);
            digitalWrite(_in4, stepHalf[i][3]);
            delay(1);
        }
    }

    inline void makeStepHalfAntiClockwise(bool direction=true) const
    {
        for (int8_t i = 7; i >= 0; i--)
        {
            digitalWrite(_in1, stepHalf[i][0]);
            digitalWrite(_in2, stepHalf[i][1]);
            digitalWrite(_in3, stepHalf[i][2]);
            digitalWrite(_in4, stepHalf[i][3]);
            delay(1);
        }
    }

    inline void makeStepNormalClockwise() const
    {
        for (int8_t i = 0; i < 4; i++)
        {
            digitalWrite(_in1, stepNormal[i][0]);
            digitalWrite(_in2, stepNormal[i][1]);
            digitalWrite(_in3, stepNormal[i][2]);
            digitalWrite(_in4, stepNormal[i][3]);
            delay(1);
        }
    }

    inline void makeStepNormalAntiClockwise() const
    {
        for (int8_t i = 3; i >= 0; i--)
        {
            digitalWrite(_in1, stepNormal[i][0]);
            digitalWrite(_in2, stepNormal[i][1]);
            digitalWrite(_in3, stepNormal[i][2]);
            digitalWrite(_in4, stepNormal[i][3]);
            delay(1);
        }
    }

    inline void makeStepWaveClockwise() const
    {
        for (int8_t i = 0; i < 4; i++)
        {
            digitalWrite(_in1, stepWave[i][0]);
            digitalWrite(_in2, stepWave[i][1]);
            digitalWrite(_in3, stepWave[i][2]);
            digitalWrite(_in4, stepWave[i][3]);
            delay(1);
        }
    }

    inline void makeStepWaveAntiClockwise() const
    {
        for (int8_t i = 3; i >= 0; i--)
        {
            digitalWrite(_in1, stepWave[i][0]);
            digitalWrite(_in2, stepWave[i][1]);
            digitalWrite(_in3, stepWave[i][2]);
            digitalWrite(_in4, stepWave[i][3]);
            delay(1);
        }
    }

public:
    ULN2003(byte in1, byte in2, byte in3, byte in4, uint32_t usPerStep, MovementType movementType=HALF)
        : _in1(in1), _in2(in2), _in3(in3), _in4(in4), _movementType(movementType)
    {
        pinMode(_in1, OUTPUT);
        pinMode(_in2, OUTPUT);
        pinMode(_in3, OUTPUT);
        pinMode(_in4, OUTPUT);

        setUsPerStep(usPerStep);
    }

    void attach(bool state)
    {
        if (state)
        {
            switch(_movementType)
            {
            case HALF:
                digitalWrite(_in1, stepHalf[0][0]);
                digitalWrite(_in2, stepHalf[0][1]);
                digitalWrite(_in3, stepHalf[0][2]);
                digitalWrite(_in4, stepHalf[0][3]);
                break;
            case NORMAL:
                digitalWrite(_in1, stepNormal[0][0]);
                digitalWrite(_in2, stepNormal[0][1]);
                digitalWrite(_in3, stepNormal[0][2]);
                digitalWrite(_in4, stepNormal[0][3]);
                break;
            case WAVE:
                digitalWrite(_in1, stepWave[0][0]);
                digitalWrite(_in2, stepWave[0][1]);
                digitalWrite(_in3, stepWave[0][2]);
                digitalWrite(_in4, stepWave[0][3]);
                break;
            default:
                break;
            }
        }
        else
        {
            digitalWrite(_in1, LOW);
            digitalWrite(_in2, LOW);
            digitalWrite(_in3, LOW);
            digitalWrite(_in4, LOW);
        }
        _attached = state;
    }

    bool setUsPerStep(uint32_t usPerStep)
    {
        if (usPerStep < MIN_US_PER_STEP)
        {
            _usPerStep = MIN_US_PER_STEP;
            SNPRINTF_FLASH(_errorStr, ULN_2003_ERROR_LOG_STR_MAX_SIZE, F("WARNING: Intentaste poner los microsegundos por paso del stepper al valor de %u cuando el minimo permitido es %u. Se seteo el tiempo entre pasos al minimo posible"), usPerStep, MIN_US_PER_STEP);
            _errorFlag = true;
            return false;
        }

        _usPerStep = usPerStep;
        return true;
    }

    bool setMsPerRevolution(uint16_t msPerRevolution)
    {
        uint32_t stepsPerRevolution = (_movementType == HALF ? STEPS_PER_REVOLUTION_WITH_GEARING*2 : STEPS_PER_REVOLUTION_WITH_GEARING);
        uint32_t usPerStep = ((uint32_t)msPerRevolution) * 1000 / stepsPerRevolution;
        return setUsPerStep(usPerStep);
    }

    void makeSteps(long steps)
    {
        if (steps == 0) return;
        bool clockwise = steps > 0;
        long n = clockwise ? steps : -steps;
        _attached = true;
        switch(_movementType)
        {
        case HALF:
            if (clockwise)
            {
                for (long i = 0; i < n; i++)
                    makeStepHalfClockwise();
            }
            else
            {
                for (long i = 0; i < n; i++)
                    makeStepHalfAntiClockwise();
            }
            break;
        case NORMAL:
            if (clockwise)
            {
                for (long i = 0; i < n; i++)
                    makeStepNormalClockwise();
            }
            else
            {
                for (long i = 0; i < n; i++)
                    makeStepNormalAntiClockwise();
            }
            break;
        case WAVE:
            if (clockwise)
            {
                for (long i = 0; i < n; i++)
                    makeStepWaveClockwise();
            }
            else
            {
                for (long i = 0; i < n; i++)
                    makeStepWaveAntiClockwise();
            }
            break;
        default:
            break;
        }
        _currPosition += steps;
    }

    void goToPosition(long position)
    {
        // position is the step number
        makeSteps(position - _currPosition);
    }

    long getCurrentPosition() const { return _currPosition; }
    bool attached() const { return _attached; }
};

#endif
