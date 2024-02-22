#ifndef STATIC_DEQUE_H
#define STATIC_DEQUE_H

// Inspired by https://github.com/Zarfab/ArduinoDeque/

#include <Arduino.h>

template <class T, size_t SIZE>
class StaticDeque
{
private:
    size_t _front, _back, _count;
    T _data[SIZE];
    T _null;

public:
    StaticDeque()
        : _front(0), _back(0), _count(0), _null(T())
    {}
    inline size_t count() const { return _count; }
    inline bool empty() const { return _count == 0; }
    inline bool isFull() const { return _count >= SIZE; }
    inline size_t front() const { return _front; }
    inline size_t back() const { return _back; }
    T peekFront() const;
    T peekBack() const;
    void clear() { _front = 0; _back = 0; _count = 0;}// memset(_data, 0, (SIZE+1)*sizeof(T)); }
    T popFront();
    T popBack();
    void pushFront(const T &val, bool dropOut=true);
    void pushBack(const T &val, bool dropOut=true);
    T& operator[](size_t index);

    // void test()
    // {
    //     printf("[");
    //     for (size_t i = 0; i < SIZE-1; i++)
    //         printf("%i, ", _data[i]);
    //     printf("%i] %lu, %lu, %lu\n", _data[SIZE-1], _front, _back, _count);
    // }
};

template <class T, size_t SIZE>
T StaticDeque<T, SIZE>::peekFront() const
{
    if (empty()) return T();
    else return _data[_front];
}

template <class T, size_t SIZE>
T StaticDeque<T, SIZE>::peekBack() const
{
    if (empty()) return T();
    else return _data[_back];
}

template <class T, size_t SIZE>
T StaticDeque<T, SIZE>::popFront()
{
    if (empty()) return T();
    else
    {
        T res = _data[_front++];
        --_count;
        if (_front > SIZE-1) _front -= SIZE;
        return res;
    }
}

template <class T, size_t SIZE>
T StaticDeque<T, SIZE>::popBack()
{
    if (empty()) return T();
    else
    {
        if (_back == 0) _back = SIZE-1;
        else --_back;
        T res = _data[_back];
        --_count;
        return res;
    }
}

template <class T, size_t SIZE>
void StaticDeque<T, SIZE>::pushFront(const T &val, bool dropOut)
{
    if (isFull())
    {
        if (dropOut)
            return; // drops out when full
        else
        {
            printf("%lu, %u\n", _count, isFull());
            if (_back == 0) _back = SIZE - 1;
            else --_back;
        }
    }
    else
    {
        ++_count;
    }
    if (_front == 0) _front = SIZE - 1;
    else --_front;
    _data[_front] = val;
}

template <class T, size_t SIZE>
void StaticDeque<T, SIZE>::pushBack(const T &val, bool dropOut)
{
    if (isFull())
    {
        if (dropOut)
            return; // drops out when full
        else
        {
            if (_front >= SIZE-1) _front = 0;
            else ++_front;
        }
    }
    else
    {
        ++_count;
    }
    _data[_back] = val;
    if (++_back > SIZE-1) _back = 0;
}

template <class T, size_t SIZE>
T& StaticDeque<T, SIZE>::operator[](size_t index)
{
    if (empty() || index > SIZE-1) return _null; // puede fallar porque quiere una referencia
    else return _data[(_front + index) % SIZE];
}

#endif