#include "stepper.hpp"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "user_panic.h"

#define STEPPER_ABS(x) ((x) < 0 ? -(x) : (x))


// step patterns

static const bool step_wave [4][4] =
{
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
};

static const bool step_normal [4][4] =
{
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1}
};

static const bool step_half [8][4] =
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
//

static bool get_step_info(enum StepType step_type, uint8_t *n_step_variants, const bool (*stepTable)[4])
{
    switch (step_type) {
        STEPPER_WAVE:
            *n_step_variants = 4;
            stepTable = step_wave;
            break;
        STEPPER_NORMAL:
            *n_step_variants = 4;
            stepTable = step_normal;
            break;
        STEPPER_HALF:
            *n_step_variants = 8;
            stepTable = step_half;
            break;
        default:
            return false;
    }
    return true;
}


inline bool Stepper::is_position_allowed(int32_t position) const
{
    return position >= _min_position && position <= _max_position;
}

inline bool Stepper::rotation_should_be_clockwise(int32_t next_position)
{
    // returns true if the stepper should turn clockwise to make <next_position> steps.
    // For this the sign of <next_position> and <_clockwise_is_forward> must be taken into account
    bool is_forward = next_position > 0;
    return (is_forward && _clockwise_is_forward) || (!is_forward && !_clockwise_is_forward);
}

bool Stepper::stepper_raw_make_steps_clockwise_blocking(int32_t steps)
{
    uint8_t n_step_variants;
    const bool (*step_table)[4];

    if (!get_step_info(_step_type, &n_step_variants, step_table))
        return false;

    for (uint32_t n = 0; n < steps; n++)
    {
        for (int8_t i = 0; i < n_step_variants; i++)
        {
            gpio_put(_pin_1, step_table[i][0]);
            gpio_put(_pin_2, step_table[i][1]);
            gpio_put(_pin_3, step_table[i][2]);
            gpio_put(_pin_4, step_table[i][3]);
            sleep_us(_step_delay_us);
        }
    }

    return true;
}

bool Stepper::stepper_raw_make_steps_anticlockwise_blocking(int32_t steps)
{
    uint8_t n_step_variants;
    const bool (*step_table)[4];

    if (!get_step_info(_step_type, &n_step_variants, step_table))
        return false;

    for (uint32_t n = 0; n < steps; n++)
    {
        for (int8_t i = n_step_variants-1; i >= 0; i--)
        {
            gpio_put(_pin_1, step_table[i][0]);
            gpio_put(_pin_2, step_table[i][1]);
            gpio_put(_pin_3, step_table[i][2]);
            gpio_put(_pin_4, step_table[i][3]);
            sleep_us(_step_delay_us);
        }
    }

    return true;
}

// typedef bool(* repeating_timer_callback_t) (repeating_timer_t *rt)
/*
struct repeating_timer {
    int64_t delay_us;
    alarm_pool_t *pool;
    alarm_id_t alarm_id;
    repeating_timer_callback_t callback;
    void *user_data;
};
*/
static bool stepper_clockwise_async_callback(repeating_timer_t *rt)
{
    struct Stepper::AsyncData *async_data = (struct Stepper::AsyncData *)rt->user_data;

    gpio_put(async_data->pin_1, async_data->step_table[async_data->last_microstep][0]);
    gpio_put(async_data->pin_2, async_data->step_table[async_data->last_microstep][1]);
    gpio_put(async_data->pin_3, async_data->step_table[async_data->last_microstep][2]);
    gpio_put(async_data->pin_4, async_data->step_table[async_data->last_microstep][3]);

    if (++async_data->last_microstep >= async_data->n_step_variants)
    {
        async_data->last_step = 0;
        ++async_data->last_step;
    }

    bool repeat = async_data->last_step < async_data->total_steps;
    if (!repeat)
        if (async_data->cb)
            async_data->cb(async_data->starting_position, async_data->end_position);
    return repeat;
}
static bool stepper_anticlockwise_async_callback(repeating_timer_t *rt)
{
    struct Stepper::AsyncData *async_data = (struct Stepper::AsyncData *)rt->user_data;

    gpio_put(async_data->pin_1, async_data->step_table[async_data->last_microstep][0]);
    gpio_put(async_data->pin_2, async_data->step_table[async_data->last_microstep][1]);
    gpio_put(async_data->pin_3, async_data->step_table[async_data->last_microstep][2]);
    gpio_put(async_data->pin_4, async_data->step_table[async_data->last_microstep][3]);

    if (--async_data->last_microstep < 0)
    {
        async_data->last_step = async_data->n_step_variants - 1;
        ++async_data->last_step;
    }

    bool repeat = async_data->last_step < async_data->total_steps;
    if (!repeat)
        if (async_data->cb)
            async_data->cb(async_data->starting_position, async_data->end_position);
    return repeat;
}

Stepper::Stepper(bool clockwise_is_forward, int32_t min_position, int32_t max_position,
            uint pin_1, uint pin_2, uint pin_3, uint pin_4,
            enum StepType step_type, uint32_t step_delay_us)
        : _pin_1(pin_1), _pin_2(pin_2), _pin_3(pin_3), _pin_4(pin_4), _step_type(step_type), _step_delay_us(step_delay_us),
          _clockwise_is_forward(clockwise_is_forward), _min_position(min_position), _max_position(max_position), _current_position(0),
          UserFlashBase(sizeof(_current_position))
    {}

void Stepper::setup_async_data(int32_t steps, bool clockwise, stepper_async_end_callback_t cb)
{
    if (_async_data.executing || !is_position_allowed(_current_position + steps)) return; // this should never happen
    _async_data.pin_1 = _pin_1;
    _async_data.pin_2 = _pin_2;
    _async_data.pin_3 = _pin_3;
    _async_data.pin_4 = _pin_4;
    _async_data.last_step = 0;
    _async_data.total_steps = (uint32_t)steps;

    // it's fine to not use volatile here because it's only supposed to execute if no async action is ocurring
    // warning: this assumes the get_step_info function always succeeds!
    get_step_info(_step_type, (uint8_t *)&_async_data.n_step_variants, (const bool (*)[4])&_async_data.step_table);

    // if it's anticlockwise, we should start at the las microstep and go backward
    _async_data.last_microstep = (!clockwise) * (_async_data.n_step_variants - 1);
    _async_data.cb = cb;
    _async_data.starting_position = _current_position;
    _async_data.end_position = _current_position + steps;
}

bool Stepper::move_steps_async(int32_t steps, stepper_async_end_callback_t callback)
{
    // returns true if the stepper movement began, false if it couldn't be started due to
    // - position not allowed
    // - no alarm available for interrupts

    if (!_init_flag) return false;

    if (_async_data.executing) return false; // can only have 1 async action simultaneously per stepper (obviously)

    if (!is_position_allowed(_current_position + steps)) return false;
    bool clockwise = rotation_should_be_clockwise(steps);

    setup_async_data(steps, clockwise, callback);

    _async_data.executing = true;
    bool res;
    if (clockwise)
    {
        res = add_repeating_timer_us(_step_delay_us, stepper_clockwise_async_callback, (void *)&_async_data, &_async_timer);
    }
    else
    {
        res = add_repeating_timer_us(_step_delay_us, stepper_anticlockwise_async_callback, (void *)&_async_data, &_async_timer);
    }
    if (!res) _async_data.executing = false;

    return res;
}
bool Stepper::move_to_position_async(int32_t next_position, stepper_async_end_callback_t callback)
{
    int32_t steps = next_position - _current_position;
    return move_steps_async(steps, callback);
}


bool Stepper::begin()
{
    uint32_t mask = (1 << _pin_1) || (1 << _pin_2) || (1 << _pin_3) || (1 << _pin_4);
    gpio_init_mask(mask);
    gpio_set_dir_out_masked(mask);
    gpio_clr_mask(mask);

    load_position_from_flash();

    _init_flag = true;
    return true;
}

bool Stepper::move_steps_blocking(int32_t steps)
{
    if (!_init_flag) return false;
    if (!is_position_allowed(_current_position + steps)) return false;
    bool clockwise = rotation_should_be_clockwise(steps);

    // I don't check if the stepper actually made all necessary steps, because if it didn't,
    // there's nothing to do that I know of. It's better to just assume that, if the movement
    // started, it should successfully end (it is also pretty safe to do so).
    if (clockwise)
    {
        stepper_raw_make_steps_clockwise_blocking(steps);
    }
    else
    {
        stepper_raw_make_steps_clockwise_blocking(steps);
    }

    _current_position += steps;

    return true;
}


bool Stepper::move_to_position_blocking(int32_t next_position)
{
    int32_t steps = next_position - _current_position;
    return move_steps_blocking(steps);
}


void Stepper::save_position_to_flash()
{
    base_flash_save(&_current_position);
}

void Stepper::load_position_from_flash()
{
    base_flash_load(&_current_position);
}