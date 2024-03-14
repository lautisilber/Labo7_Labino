#ifndef SMART_SERIAL_H
#define SMART_SERIAL_H

#include <Arduino.h>

#define MAX_COMMANDS 64
#define MAX_ARGUMENTS 16 // lim of args is actually MAX_ARGUMENTS-1 since the first argument is actually the command
#define STREAM_BUFFER_LEN 64


typedef struct CommandArguments
{
private:
    char *const *const _args;

public:
    const size_t N = 0;

    CommandArguments(size_t n, const char *const args[MAX_ARGUMENTS])
        : N(n), _args(args)
    {}

    const char *arg(size_t n) const
    {
        if (n >= N) return NULL;
        return _args[n];
    }

    bool toInt(size_t n, long *i)
    {
        const char *str = arg(n);
        if (str == NULL) return false;

        char *endptr;
        *i = strtol(str, &endptr, 10);
        return str[0] != '\0' && *endptr == '\0';
    }

    bool toBool(size_t n, bool *b)
    {
        const char *str = arg(n);
        if (str == NULL) return false;

        if (
            strcmp_P(str, (PGM_P)F("1")) == 0 || strcmp_P(str, (PGM_P)F("true")) == 0 ||
            strcmp_P(str, (PGM_P)F("True")) == 0 || strcmp_P(str, (PGM_P)F("TRUE")) == 0
        )
        {
            *b = true;
            return true;
        }

        if (
            strcmp_P(str, (PGM_P)F("0")) == 0 || strcmp_P(str, (PGM_P)F("false")) == 0 ||
            strcmp_P(str, (PGM_P)F("False")) == 0 || strcmp_P(str, (PGM_P)F("FALSE")) == 0
        )
        {
            *b = false;
            return true;
        }
        return false;
    }
} CommandArguments;


typedef void (*smartCommandCB_t)(Stream*, CommandArguments*);
typedef void (*serialDefaultCommandCB_t)(Stream*, const char *cmd);

static void __defaultCommandNotRecognizedCB(Stream *stream, const char *cmd)
{
    stream->print(F("ERROR: No se reconoce el comando \""));
    stream->print(cmd);
    stream->println(F("\""));
}

class SmartCommandBase
{
protected:
    const char *com;
public:
    smartCommandCB_t cb;

    virtual bool compare(const char *str) const = 0;
};

class SmartCommandP : public SmartCommandBase
{
public:
    SmartCommandP(const char *command, smartCommandCB_t callback)
    {
        cb = callback;
        com = command;
    }

    bool compare(const char *str) const { return strcmp(str, com) == 0; }
};
// created to match the CreateSmartCommandF macro
#define CreateSmartCommandP(class_name, command, callback) \
    const char __##class_name##_pgm_cmd[] = command; \
    SmartCommandP class_name(__##class_name##_pgm_cmd, callback);

class SmartCommandF : public SmartCommandBase
{
public:
    SmartCommandF(const __FlashStringHelper *command, smartCommandCB_t callback)
    {
        cb = callback;
        com = reinterpret_cast<PGM_P>(command);
    }
    SmartCommandF(const char *command, smartCommandCB_t callback)
    {
        cb = callback;
        com = command;
    }

    bool compare(const char *str) const { return strcmp_P(str, com) == 0; }
};
// this macro can be used to create a SmartCommandF without the need to create the const PROGMEM char[] variable manually
// this will work as if SmartCommandF class_name(F(command), callback) were possible
#define CreateSmartCommandF(class_name, command, callback) \
    const PROGMEM char __##class_name##_pstr_cmd[] = command; \
    SmartCommandF class_name(__##class_name##_pstr_cmd, callback);

static char *__trimChar(char *str, char c)
{
    // this modifies the original str. it returns a new pointer (inside the original str)
    // https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if ( c == '\0' ) { return NULL; } // c cant be '\0'
    if( str == NULL ) { return NULL; }
    if( str[0] == '\0' ) { return str; }

    len = strlen(str);
    endp = str + len;

    while ( *frontp == c ) { ++frontp; } // move front pointer forward until its not c anymore
    if( endp != frontp ) // move end pointer backward until its not c anymore or matches front pointer
    {
        while( *(--endp) == c && endp != frontp ) {}
    }

    if( str + len - 1 != endp ) // if str left, end it eith '\0'
        *(endp + 1) = '\0';

    return frontp;
}

static void __removeConsecutiveDuplicates(char *str, char c)
{
    // https://www.includehelp.com/c-programs/remove-consecutive-repeated-characters-from-string.aspx
    if (str == NULL) return;

    bool lastCharIsC = str[0] == c;
    size_t len = strlen(str);
    
    // assign 0 to len1 - length of removed characters
    size_t len1 = 0;
 
    // Removing consecutive repeated characters from string
    for(size_t i = 0; i < (len-len1);)
    {
        if(str[i]==str[i+1] && str[i] == c)
        {
            /*shift all characters*/
            for(size_t j = i; j < (len-len1); j++)
                str[j]=str[j+1];
            len1++;
        }
        else
        {
            i++;
        }
    }
}

class SmartSerial
{
private:
    Stream *_stream = NULL;
    size_t _nCommands = 0;
    SmartCommandBase *_coms[MAX_COMMANDS] = {0};
    char _buffer[STREAM_BUFFER_LEN+1] = {'\0'};
    size_t _bufferPos = 0;
    char _endChar;
    char _sepChar;
    serialDefaultCommandCB_t _defaultCB;

public:
    SmartSerial(Stream *const stream, char endChar='\n', char sepChar=' ')
        : _stream(stream), _endChar(endChar), _sepChar(sepChar), _defaultCB(__defaultCommandNotRecognizedCB)
    {}

    bool addCommand(const SmartCommandBase *const command)
    {
        if (_nCommands >= MAX_COMMANDS-1)
            return false;
        _coms[_nCommands++] = command;
        return true;
    }

    void setDefaultCallback(serialDefaultCommandCB_t cb=__defaultCommandNotRecognizedCB)
    {
        _defaultCB = cb;
    }

    void tick()
    {
        if (_stream == NULL) return;
        if (!_stream->available()) return;
        while (_stream->available())
        {
            char c = _stream->read();

            if (c == _endChar)
            {
                // trim leading and trailing sepChars from _buffer
                char *trimmedBuffer = __trimChar(_buffer, _sepChar);
                // remove duplicate sepChars
                __removeConsecutiveDuplicates(trimmedBuffer, _sepChar);
                size_t trimmedBufferLen = strlen(trimmedBuffer);

                if (trimmedBuffer != NULL && trimmedBufferLen > 0)
                {

                    // find all arguments
                    char *sepPtr = trimmedBuffer; // ptr of the las separation char found + 1
                    char *args[MAX_ARGUMENTS] = {0}; // ptrs of all separation characters found
                    size_t nArgs = 0; // amount of arguments
                    char *ptr;
                    while (true)
                    {
                        ptr = strchr(sepPtr, _sepChar); // this returns the pointer to the first ocurrence

                        if (ptr == NULL) break;

                        // later we split the buffer into several strings,
                        // each being an argument (the first is the command)
                        // to reuse the same char array, we can change the
                        // sep chars for '\0' and save pointers to the next char
                        *ptr = '\0';
                        
                        // -2 because we dont want sepIndex to point to the ending '\0'
                        if (ptr >= trimmedBuffer+trimmedBufferLen-1) break;
                        sepPtr = ptr+1;
                        
                        if (nArgs >= MAX_ARGUMENTS) break;
                        args[nArgs++] = sepPtr;
                    }
                    // the arguments are already separated by \0
                    // this makes it so that the command string is actually the trimmedBuffer pointer
                    const char *command = trimmedBuffer;

                    // Serial.println(nArgs);
                    // Serial.print("Command: ");
                    // Serial.println(command);
                    // for (size_t i = 0; i < nArgs; i++)
                    // {
                    //     Serial.println(args[i]);
                    // }

                    // get the serial command selected
                    SmartCommandBase *sc = NULL;
                    for (size_t i = 0; i < _nCommands; i++)
                    {
                        if (_coms[i]->compare(command)) //(strcmp(_coms[i]->com, command) == 0)
                        {
                            sc = _coms[i];
                            break;
                        }
                    }
                    if (sc == NULL)
                        _defaultCB(_stream, command);
                    else
                    {
                        CommandArguments comArgs(nArgs, args);
                        sc->cb(_stream, &comArgs);
                    }
                }

                memset(_buffer, '\0', STREAM_BUFFER_LEN);
                _bufferPos = 0;
            }
            else
            {
                if (_bufferPos < STREAM_BUFFER_LEN-1)
                    _buffer[_bufferPos++] = c;
            }
        }
    }   
};


#endif