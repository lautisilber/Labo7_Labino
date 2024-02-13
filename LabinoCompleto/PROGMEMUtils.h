#ifndef PROGMEM_UTILS_H
#define PROGMEM_UTILS_H

#include <Arduino.h>
#include <avr/pgmspace.h>

// https://www.nongnu.org/avr-libc/user-manual/group__avr__pgmspace.html#ga963f816fc88a5d8479c285ed4c630229
// PGM_P -> Used to declare a variable that is a pointer to a string in program space.
// Es el tipo de variable de un char pointer de un texto que vive en PROGMEM
void progmemToStream(PGM_P p_charr, Stream *stream)
{
    char c;
    for (size_t i = 0; i < strlen_P(p_charr); i++) {
        c = pgm_read_byte_near(p_charr + i);
        stream->print(c);
    }
}

void progmemToStack(PGM_P p_charr, char *charr, size_t charrLen)
{
    size_t len = min(strlen_P(p_charr), charrLen);
    memccpy_P(charr, p_charr, '\0', len);
}

void progmemToStack(const __FlashStringHelper *fsh_charr, char *charr, size_t charrLen)
{
    PGM_P p_charr = reinterpret_cast<PGM_P>(fsh_charr);
    size_t len = min(strlen_P(p_charr), charrLen);
    memccpy_P(charr, p_charr, '\0', len);
}

#define SNPRINTF_PROGMEM_NO_ARGS(dest, destLen, src_p)                 \
{                                                                      \
    const size_t __bufferLen = strlen_P(src_p);                        \
    char __buffer[__bufferLen+1] = {0};                                \
    progmemToStack(src_p, __buffer, __bufferLen);                      \
    snprintf(dest, destLen, __buffer);                                 \
}
#define SNPRINTF_PROGMEM(dest, destLen, src_p, ...)                    \
{                                                                      \
    const size_t __bufferLen = strlen_P(src_p);                        \
    char __buffer[__bufferLen+1] = {0};                                \
    progmemToStack(src_p, __buffer, __bufferLen);                      \
    snprintf(dest, destLen, __buffer, __VA_ARGS__);                    \
}

#define SNPRINTF_FLASH_NO_ARGS(dest, destLen, src_fsh)                 \
{                                                                      \
    PGM_P __src_p = reinterpret_cast<PGM_P>(src_fsh);                  \
    const size_t __bufferLen = strlen_P(__src_p);                      \
    char __buffer[__bufferLen+1] = {0};                                \
    progmemToStack(__src_p, __buffer, __bufferLen);                    \
    snprintf(dest, destLen, __buffer);                                 \
}
#define SNPRINTF_FLASH(dest, destLen, src_fsh, ...)                    \
{                                                                      \
    PGM_P __src_p = reinterpret_cast<PGM_P>(src_fsh);                  \
    const size_t __bufferLen = strlen_P(__src_p);                      \
    char __buffer[__bufferLen+1] = {0};                                \
    progmemToStack(__src_p, __buffer, __bufferLen);                    \
    snprintf(dest, destLen, __buffer);                                 \
}


#endif