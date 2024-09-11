#include <stdio.h>
#include <string.h>
#include <stdint.h>

void remove_chars(char *s, char c)
{
    size_t writer = 0, reader = 0;

    while (s[reader])
    {
        if (s[reader]!=c) 
            s[writer++] = s[reader];
        ++reader;       
    }
    s[writer] = '\0';
}

static void remove_non_alphanumeric_chars(char *s)
{
    size_t writer = 0, reader = 0;

    while (s[reader])
    {
        if ((s[reader] >= 'A' && s[reader] <= 'Z') ||
            (s[reader] >= 'a' && s[reader] <= 'z') ||
            (s[reader] >= '0' && s[reader] <= '9'))
            s[writer++] = s[reader];
        ++reader;       
    }
    s[writer] = '\0';
}

int main()
{
    const char o[] = "hi ho\tw a44r\ne6 y4446ou64?\0";
    char s[strlen(o)];

    memcpy(s, o, strlen(o));

    printf("%s | %lu\n", s, strlen(o));
    
    remove_chars(s, '4');
    remove_chars(s, '6');
    remove_chars(s, '\n');
    remove_chars(s, '\t');
    printf("%s | %lu\n", s, strlen(s));

    memcpy(s, o, strlen(o));
    remove_non_alphanumeric_chars(s);
    printf("%s | %lu\n", s, strlen(s));

    return 0;
}