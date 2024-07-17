#include "utils.hpp"
#include <numeric>
#include <math.h>
#include <algorithm>


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


template <typename T_SAMPLES, typename T_RESULT>
size_t interquartile_range_filter(T_SAMPLES *arr, size_t len, T_RESULT mult)
{
    // TODO: test this

    // this filters outliers by interquartile range filtering
    // all happens in-place, and the new filtered array has the same
    // starting pointer, but is same length or shorter. The new length of
    // the filtered array is returned by the function

    const T_SAMPLES* begin = arr;
    const T_SAMPLES* end = begin + len;
    
    // find quartiles
    const size_t q1_index = len / 4;
    const size_t q2_index = len / 2;
    const size_t q3_index = q1_index + q2_index; // = 3 * len / 4 = 2*len / 4 + len / 4 = len / 2 + len / 4 = q2_index + q1_index

    // std::neth_element(begin, nth, end)
    // Rearranges the elements in the range [first,last), in such a way that the element
    // at the nth position is the element that would be in that position in a sorted sequence.
    // std::nth_element is ~O(n) whereas std::sort  is ~O(n logn), so it's actually better to use nth_element
    // https://stackoverflow.com/questions/11964552/finding-quartiles
    std::nth_element(begin, begin+q1_index, end);
    std::nth_element(begin+q1_index+1, begin+q2_index, end);
    std::nth_element(begin+q2_index+1, begin+q3_index, end);
    T_RESULT q1 = static_cast<T_RESULT>(arr[q1_index]);
    T_RESULT q2 = static_cast<T_RESULT>(arr[q2_index]);
    T_RESULT q3 = static_cast<T_RESULT>(arr[q3_index]);

    // interquantile range
    T_RESULT iqr = q3 - q1;

    // define lower and upper bounds
    T_RESULT lower_bound = q1 - (mult * iqr);
    T_RESULT upper_bound = q3 + (mult * iqr);

    // filter the data based on the bounds
    // we could avoid using remove_if if the list were already sorted. However, remove_if is ~O(n)
    // and 4*n < n*log(n), so we are still going faster (for any n > 24)
    auto outside_range = [&lower_bound, &upper_bound](T_SAMPLES val){ 
        const T_RESULT val_cast = static_cast<T_RESULT>(val);
        return val_cast < lower_bound || val_cast > upper_bound;
    };
    T_SAMPLES* new_end = std::remove_if(begin, end, outside_range);

    size_t new_len = new_end - begin;
    return new_len;
}