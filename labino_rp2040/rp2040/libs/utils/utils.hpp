#ifndef UTILS_H
#define UTILS_H


#include "pico/stdlib.h"
#include <limits.h>

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

#endif /* UTILS_H */