#ifndef COMM_UART_HPP
#define COMM_UART_HPP

#include "comm_api.hpp"
#include <stdio.h>

#define COMM_UART_ID          uart0
#define COMM_UART_BAUD_RATE   115200
#define COMM_UART_DATA_BITS   8
#define COMM_UART_STOP_BITS   1
#define COMM_UART_PARITY      UART_PARITY_NONE

#if !__has_include("FreeRTOS.h")
#include "FreeRTOS.h"
#include "task.h"
#endif

void comm_stdio_interrupt_callback_wrapper(void *api_ptr);

class CommAPI_stdio : public CommAPI
{
public:
    CommAPI_stdio(bool init, char end_char, char sep_char) : CommAPI(end_char, sep_char), _uart_inst(uart_inst)
    {
        if (init)
            stdio_init_all();
    }

    void set_rx_interrupt()
    {
        // Set up a RX interrupt
        stdio_set_chars_available_callback(comm_stdio_interrupt_callback_wrapper, this);
    }

    bool send_message(const char *msg)
    {
        if (!uart_is_writable(_uart_inst)) return false;
        uart_puts(_uart_inst, msg);
        stdio_flush();
        return true;
    }
}

void comm_stdio_interrupt_callback_wrapper(void *api_ptr)
{
    CommAPI_stdio *api = (CommAPI_stdio *)api_ptr;
    const uint32_t timeout_us = 1000;
    for (;;)
    {
        int c = stdio_getchar_timeout_us(timeout_us);
        if (c == PICO_ERROR_TIMEOUT) break;

        api->_incoming_buffer[api->_incoming_buffer_curr_index++] = c;
        
        if (c = api->_end_char || api->_incoming_buffer_curr_index >= COMM_API_READ_BUFFER-2)
        {
            api->_incoming_buffer[COMM_API_READ_BUFFER-1] = '\0';
            api->save_new_message(api->_incoming_buffer);
            api->_incoming_buffer_curr_index = 0;
            break;
        }
    }
}

/* Example of how to set up the CommAPI_stdio with rx interrupts
 *
 * CommAPI_stdio api(true, '\0', ' ');
 * ...
 * void main()
 * {
 *     api.set_rx_interrupt();
 * }
 * 
*/

#endif /* COMM_UART_HPP */