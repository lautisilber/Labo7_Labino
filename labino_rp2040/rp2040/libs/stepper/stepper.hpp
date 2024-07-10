#ifndef STEPPER_HPP
#define STEPPER_HPP

#include "pico/stdlib.h"
#include "pico/time.h"


// #define STEPPER_STEP_MIN_DELAY_US 1000

enum StepType : uint8_t
{
    STEPPER_WAVE,
    STEPPER_NORMAL,
    STEPPER_HALF
};

class Stepper
{
private:
    uint _pin_1, _pin_2, _pin_3, _pin_4;
    enum StepType _step_type;
    uint32_t _step_delay_us;
    bool _init_flag = false;
    int32_t _min_position, _max_position;
    int32_t _current_position;
    bool _attached = false;
    bool _clockwise_is_forward;

private:
    inline bool is_position_allowed(int32_t position) const;
    inline bool rotation_should_be_clockwise(int32_t next_position);
    
    bool stepper_raw_make_steps_clockwise_blocking(int32_t steps);
    bool stepper_raw_make_steps_anticlockwise_blocking(int32_t steps);

public: // async data (shouldn't be accessed by user)
    struct AsyncData
    {
        uint pin_1, pin_2, pin_3, pin_4;
        int8_t last_microstep;
        uint32_t last_step;
        uint32_t total_steps;
        // for the chosen step_type
        uint8_t n_step_variants;
        const bool (*step_table)[4];
        //
        bool executing;
    };
    volatile struct AsyncData _async_data = {0};
private:
    repeating_timer_t _async_timer;
    void setup_async_data(int32_t steps, bool clockwise);

public:
    Stepper(bool clockwise_is_forward, int32_t min_position, int32_t max_position,
            uint pin_1=15, uint pin_2=14, uint pin_3=13, uint pin_4=12,
            enum StepType step_type=STEPPER_HALF,uint32_t step_delay_us=1000)
        : _pin_1(pin_1), _pin_2(pin_2), _pin_3(pin_3), _pin_4(pin_4), _step_type(step_type), _step_delay_us(step_delay_us),
          _clockwise_is_forward(clockwise_is_forward), _min_position(min_position), _max_position(max_position), _current_position(0)
    {}

    void begin();

    bool move_steps_blocking(int32_t steps);
    bool move_to_position_blocking(int32_t next_position);

    bool move_steps_async(int32_t steps);
    bool move_to_position_async(int32_t next_position);

    inline int32_t get_current_position() const { return _current_position; }


};

#endif /* STEPPER_HPP */