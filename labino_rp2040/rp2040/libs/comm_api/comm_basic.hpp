#ifndef COMM_BASIC_HPP
#define COMM_BASIC_HPP

#include "comm_api.hpp"

class CommAPI_Basic : public CommAPI
{
public:
    CommAPI_Basic(char end_char, char sep_char) : CommAPI(end_char, sep_char) {}
    bool send_message(const char *msg)
    {
        printf("CommAPI_Basic: %s\n", msg);
        return true;
    }
};

#endif /* COMM_BASIC_HPP */