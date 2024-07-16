#include "utils.hpp"
// #include <algorithm>
#include <numeric>
// #include <iterator>
#include <math.h>
#include "pico/divider.h"


// CHAR_BIT: found in <limits.h>. The size in bits of type is simply CHAR_BIT * sizeof(type)
template <typename T>
void dec_2_bin(T dec, bool bin[sizeof(T)*CHAR_BIT])
{
    for (uint8_t i = 0; i < sizeof(T)*CHAR_BIT; ++i)
    {
        bin[i] = (dec >> (sizeof(T)*CHAR_BIT-1-i)) & 1;
    }
}


template <typename T_SAMPLES, typename T_RESULT>
void calc_mean_stdev_online(T_SAMPLES* vals, size_t n, T_RESULT *mean, T_RESULT *stdev)
{
    // https://stackoverflow.com/questions/33268513/calculating-standard-deviation-variance-in-c
    if (n == 0)
    {
        *mean = 0.0;
        *stdev = 0.0;
        return;
    }

    const T_SAMPLES* begin = vals;
    const T_SAMPLES* end = begin + n;

    // Calculate the mean
    *mean = std::accumulate<const T_SAMPLES*, T_RESULT>(begin, end, 0.0f) / n;

    // Now calculate the variance
    auto variance_func = [mean, &n](T_RESULT accumulator, const T_SAMPLES& val) {
        return accumulator + ((val - *mean)*(val - *mean) / (n - 1));
    };
    T_RESULT variance = std::accumulate<const T_SAMPLES*, T_RESULT>(begin, end, 0.0f, variance_func);

    // Finally calculate standard deviation
    *stdev = sqrt(variance);
}


template <typename T_SAMPLES, typename T_RESULT>
void calc_mean_stdev_welford(struct WelfordAggregate<T_RESULT> *existing_aggregate, T_SAMPLES new_sample)
{
    ++existing_aggregate->count;
    T_RESULT delta = new_sample - existing_aggregate->mean;
    existing_aggregate->mean += delta / existing_aggregate->count;
    T_RESULT delta2 = new_sample - existing_aggregate->mean;
    existing_aggregate->m2 += delta * delta2;
}

template <typename T_SAMPLES, typename T_RESULT>
bool calc_mean_stdev_welford_finish(struct WelfordAggregate<T_RESULT> *existing_aggregate, T_RESULT *mean, T_RESULT *stdev)
{
    if (existing_aggregate->count < 2)
    {
        *mean = 0;
        *stdev = 0;
        return false;
    }

    *mean = existing_aggregate->mean;
    float variance = existing_aggregate->m2 / existing_aggregate->count;
    float sample_variance = existing_aggregate->m2 / (existing_aggregate->count-1);
    *stdev = sqrt(sample_variance);
    return true;
}


// template <typename T>
// size_t remove_by_mask(T *arr, bool *mask, size_t len)
// {
//     size_t last_valid = 0;
//     size_t new_len = 0;

//     for (size_t i = 0; i < len; i++)
//     {
//         if (mask[i])
//         {
//             arr[last_valid++] = arr[i];
//             ++new_len;
//         }
//     }

//     return new_len;
// }

template <typename T>
size_t remove_by_mask(T *arr, bool *mask, size_t len)
{
    /*
    *  Takes an array and a bool mask. Manipulates the array such that there is a new array of length equal
    *  to the number of true in the mask that contains the values of the original array  that correspond
    *  to the true value indices of the mask (ordered). Returns the value of the new array
    */
    T* last_valid = arr;
    size_t new_len = 0;

    for (size_t i = 0; i < len; ++i)
    {
        if (mask[i])
        {
            *last_valid++ = arr[i];
            ++new_len;
        }
    }

    return new_len;
}