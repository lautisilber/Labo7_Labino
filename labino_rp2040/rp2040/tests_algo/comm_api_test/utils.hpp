#pragma once

#include <stdio.h>
#define MIN(A, B) (A > B ? B : A)
#define WARN_PRINTFLN(...) printf(__VA_ARGS__)

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