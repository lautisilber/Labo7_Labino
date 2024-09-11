#ifndef COMM_API_HPP
#define COMM_API_HPP


#include "pico/stdlib.h"
#include <string.h>
#include <assert.h>
#include "debug_helper.h"


#define COMM_API_READ_BUFFER       256
#define COMM_API_MAX_ARGUMENTS     16
#define COMM_API_FORCE_END_CHAR    true
#define COMM_API_MESSAGES_BUFFER   8

static_assert(COMM_API_MAX_ARGUMENTS >= 1);

/*
    Only ascii chars are allowed
*/


static void remove_non_important_chars(char *s, char sep_char)
{
    // removes all characters that are not end_char (already supposed to be '\0'), sep_char or alphanumeric
    size_t writer = 0, reader = 0;

    while (s[reader])
    {
        if ((s[reader] >= 'A' && s[reader] <= 'Z') ||
            (s[reader] >= 'a' && s[reader] <= 'z') ||
            (s[reader] >= '0' && s[reader] <= '9') ||
            s[reader] == end_char || s[reader] == sep_char)
            s[writer++] = s[reader];
        ++reader;       
    }
    s[writer] = '\0';
}

class CommMsg
{
private:
    char _msg[COMM_API_READ_BUFFER] = {0};
    const char _end_char, sep_char;

    // each argument_idx represents an argument. _arguments[i] has a pointer to the starting character of the argument i
    // inside the _msg. all arguments will be separated by '\0' so they act like completely different strings
    char (*_arguments)[COMM_API_MAX_ARGUMENTS] = {nullptr};
    size_t _n_arguments;

    // void reset_arguments()
    // {
    //     // check if size of _arguments can be calculated as sizeof(_arguments)
    //     memset(_arguments, nullptr, sizeof(char *) * COMM_API_MAX_ARGUMENTS);
    //     _n_arguments = 0;
    // }
    void process_msg()
    {
        // find first end_char in msg and turn it into \0 to mark the end of the msg (effectively replacing end_char with '\0')
        char *last_char = &_msg[MIN(strlen(_msg), COMM_API_READ_BUFFER)-1]; // pointer to the last character of _msg
        char *end_char strchr(_msg, _end_char); // pointer to the first end_char in _msg
        if (end_char == NULL)
        {
            #if COMM_API_FORCE_END_CHAR
            WARN_PRINTFLN("Mesage didn't have an ending char (%c). Inserting end_char in last position (see COMM_API_FORCE_END_CHAR flag)", _end_char);
            _msg[msg_len-1] = '\0';
            #else
            WARN_PRINTFLN("Mesage didn't have an ending char (%c). Can't process message", _end_char);
            reset_arguments();
            return;
            #endif
        }
        else
        {
            end_char = '\0';
        }
        /////


        // remove uninportant chars (non end (already supposed to be '\0'), sep or alphanumeric)
        remove_non_important_chars(_msg, _sep_char);
        // recalc last_char
        last_char = &_msg[strlen(_msg)-1];
        /////

        // split msg in arguments
        char *msg_char = _msg;
        for (_n_arguments = 0; _n_arguments < COMM_API_MAX_ARGUMENTS; _n_arguments++)
        {
            while (msg_char != sep_char || msg_char != '\0')
                ++msg_char;

            // if we hit an important character (sep or end)
            if (msg_char == '\0') // if end_char, dissable last argument and break
            {
                return;
            }
            else // hit sep
            {
                msg_char = '\0'; // turn sep into '\0' to separate arguments
                while (msg_char+1 == _sep_char) // ignore all repetitions of sep
                {
                    if (++msg_char == last_char) // increment msg_char. if it becomes last_char, break
                    {
                        return;
                    }
                }

                // record last char of argument (which is the same as next argument's start)
                _arguments[_n_arguments] = msg_char;
            }
        }
        /////
    }
public:
    CommMsg(const char *msg, char end_char, char sep_char) :
        _end_char(end_char), _sep_char(sep_char)
    {
        // msg as to be NULL ended
        strlcpy(_msg, msg, COMM_API_READ_BUFFER);
        process_msg();
    }

    // although internally the command is treated as the first argument, it is not exposed this way.
    // the command has its own class and argument 0 points to the internal argument 1
    size_t get_n_arguments() const { return _n_arguments-1; }
    const char *get_argument(size_t n) const
    {
        if (n < _n_arguments-1) return nullptr;
        return (const char *)_arguments[n+1];
    }
    const char *get_command() const
    {
        return get_argument(0);
    }
};


class CommInterfaceBase
{
protected:
    char _end_char, _sep_char;
public:
    CommInterfaceBase(char end_char, char sep_char) :
        _end_char(end_char), _sep_char(sep_char)
    {}
    ~CommInterfaceBase() {}
    virtual bool uint32_t write_single(char c) = 0;
    size_t write(const char *msg, size_t length, bool add_end_char=true)
    {
        size_t i = 0;
        for (i = 0; i < length; i++)
            if (!write_single(msg[i])) break;
        if (add_end_char)
            if (write_single(_end_char)) ++i;
        return i;
    }
    virtual size_t read_single(const char *msg, size_t length) = 0;
    size_t read()
}

class CommAPI
{
private:
    CommMsg _msgs[COMM_API_MESSAGES_BUFFER];
public:
    CommAPI(/* args */);
    ~CommAPI();
};

CommAPI::CommAPI(/* args */)
{
}

CommAPI::~CommAPI()
{
}


#endif /* COMM_API_HPP */