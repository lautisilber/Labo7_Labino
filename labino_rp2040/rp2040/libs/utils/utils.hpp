#ifndef UTILS_H
#define UTILS_H


#include "pico/stdlib.h"
#include <limits.h>

#define CLAMP(x, min, max) MAX(MIN(x, max), min)

template <typename T>
void dec_2_bin(T dec, bool bin[sizeof(T)*CHAR_BIT]);

template <typename T>
constexpr size_t get_bit_size() { return sizeof(T) * CHAR_BIT; }


/*
    for example, T_SAMPLES can be int32_t if the sensor's data are integers
    and T_RESULT can be float or double, depending on the precision one needs
*/
template <typename T_SAMPLES, typename T_RESULT>
void calc_mean_stdev_online(T_SAMPLES* vals, size_t n, T_RESULT *mean, T_RESULT *stdev);



template <typename T_RESULT>
struct WelfordAggregate
{
    size_t count = 0;
    T_RESULT mean = 0;
    T_RESULT m2 = 0;
};
template <typename T_SAMPLES, typename T_RESULT>
void calc_mean_stdev_welford(struct WelfordAggregate<T_RESULT> *existing_aggregate, T_SAMPLES new_sample);
template <typename T_SAMPLES, typename T_RESULT>
bool calc_mean_stdev_welford_finish(struct WelfordAggregate<T_RESULT> *existing_aggregate, T_RESULT *mean, T_RESULT *stdev);



template <typename T>
size_t remove_by_mask(T *arr, bool *mask, size_t len);

template <typename T_SAMPLES, typename T_RESULT>
size_t interquartile_range_filter(T_SAMPLES *arr, size_t len, T_RESULT mult);



template <typename T>
void linear_error_propagation(T *mean, T*stdev, T r, T r_stdev, T s, T s_stdev, T o, T o_stdev);

template <typename T>
void exponential_error_propagation(T *mean, T*stdev, T r, T r_stdev, T a, T a_stdev, T b, T b_stdev, T c, T c_stdev, T d, T d_stdev);

template <typename T>
void logarithm_error_propagation(T *mean, T*stdev, T r, T r_stdev, T a, T a_stdev, T b, T b_stdev, T c, T c_stdev, T d, T d_stdev);




template <typename T_ARGS, typename T_CALC>
T_ARGS map_2_range(T_ARGS value, T_ARGS in_min, T_ARGS in_max, T_ARGS out_min, T_ARGS out_max);

template <typename T>
int8_t sign(T v);

template <typename T>
inline T abs(T v);

// returns the number of steps to make a linear interpolation beteween begin and end with a step of step_size
template <typename T>
size_t lerp(T begin, T end, T step_size);

// FIFOStackForClasses
// This works like a FIFO, but it's specifically ddesigned for it's use with classes (T type is a class)
// Using classes makes the method push weird to implement, because it's arguments may vary from class to class,
// so in this implementation, the FIFO buffer is filled and slots are enabled or disabled (instead of actually
// pushing and popping). This allows easier manipulation with a peek method of the classes in the buffer.
// With this approach, a class whose slot was just enabled, should be edited instantly after. Otherwise,
// the class corresponding to that slot will hold the data last used.
// To this goal, peek_back_to_edit should be used directly after add_slot_back; and peek_front_to_read should be
// used directly before remove_slot_front
template <typename T, size_t N>
class FIFOStackForClasses
{
private:
    T _data[N];
    size_t _front_index, _rear_index;
    size_t count;

public:
    FIFOStackForClasses() : _front_index(0), _rear_index(0), count(0) {}
    bool empty() const { return count == 0; }
    bool full() const { return count == N; }
    size_t size() const { return count; }
    size_t capacity() const { return N; }

    bool add_slot_back()
    {
        if (full()) return false;
        _rear_index = (_rear_index + 1) % N;  // Circular indexing
        ++count;
        return true;
    }

    bool remove_slot_front()
    {
        if (empty()) return false;
        _rear_index = (_rear_index + 1) % N;
        --count;
        return true;
    }

    T *peek_back_to_edit() { return &_data[_rear_index]; }
    T *peek_front_to_read() { return &_data[_front_index]; }
};

#endif /* UTILS_H */