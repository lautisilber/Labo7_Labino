#include "comm_api.hpp"

static void remove_non_important_chars(char *s, char sep_char)
{
    // removes all characters that are not end_char (already supposed to be '\0'), sep_char or alphanumeric
    size_t writer = 0, reader = 0;

    while (s[reader])
    {
        if ((s[reader] >= 'A' && s[reader] <= 'Z') ||
            (s[reader] >= 'a' && s[reader] <= 'z') ||
            (s[reader] >= '0' && s[reader] <= '9') ||
            s[reader] == '\0' || s[reader] == sep_char)
            s[writer++] = s[reader];
        ++reader;       
    }
    s[writer] = '\0';
}
void CommAPI::CommMsg::set_raw_msg(const char *msg)
{
    strlcpy(_msg, msg, COMM_API_READ_BUFFER);
}

void CommAPI::CommMsg::process_msg(char end_char, char sep_char)
{
    // find first end_char in msg and turn it into \0 to mark the end of the msg (effectively replacing end_char with '\0')
    char *last_char_ptr = &_msg[MIN(strlen(_msg), COMM_API_READ_BUFFER)-1]; // pointer to the last character of _msg
    char *end_char_ptr = strchr(_msg, end_char); // pointer to the first end_char in _msg
    if (end_char_ptr == NULL)
    {
        #if COMM_API_FORCE_END_CHAR
        WARN_PRINTFLN("Mesage didn't have an ending char (%c). Inserting end_char in last position (see COMM_API_FORCE_END_CHAR flag)", end_char);
        *last_char_ptr = '\0';
        #else
        WARN_PRINTFLN("Mesage didn't have an ending char (%c). Can't process message", end_char);
        reset_arguments();
        return;
        #endif
    }
    else
    {
        *end_char_ptr = '\0';
    }
    /////


    // remove uninportant chars (non end (already supposed to be '\0'), sep or alphanumeric)
    remove_non_important_chars(_msg, sep_char);
    // recalc last_char_ptr
    last_char_ptr = &_msg[strlen(_msg)-1];
    /////

    // split msg in arguments
    char *msg_char_ptr = _msg;
    for (_n_arguments = 0; _n_arguments < COMM_API_MAX_ARGUMENTS; _n_arguments++)
    {
        while (*msg_char_ptr != sep_char || *msg_char_ptr != '\0')
            ++msg_char_ptr;

        // if we hit an important character (sep or end)
        if (*msg_char_ptr == '\0') // if end_char, dissable last argument and break
        {
            return;
        }
        else // hit sep
        {
            *msg_char_ptr = '\0'; // turn sep into '\0' to separate arguments
            while (*(msg_char_ptr+1) == sep_char) // ignore all repetitions of sep
            {
                if (++msg_char_ptr == last_char_ptr) // increment msg_char_ptr. if it becomes last_char_ptr, break
                {
                    return;
                }
            }

            // record last char of argument (which is the same as next argument's start)
            _arguments[_n_arguments] = msg_char_ptr;
        }
    }
    /////
}