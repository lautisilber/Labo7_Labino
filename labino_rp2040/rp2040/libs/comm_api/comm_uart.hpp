#ifndef COMM_UART_HPP
#define COMM_UART_HPP

#include "comm_api.hpp"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define COMM_UART_ID          uart0
#define COMM_UART_BAUD_RATE   115200
#define COMM_UART_DATA_BITS   8
#define COMM_UART_STOP_BITS   1
#define COMM_UART_PARITY      UART_PARITY_NONE


class CommAPI_UART : public CommAPI
{
public:
    // not to be used by user
    uart_inst_t _uart_inst;
public:
    CommAPI_UART(uart_inst_t uart_inst, uint baudrate, uint tx_pin, uint rx_pin, char end_char, char sep_char) : CommAPI(end_char, sep_char), _uart_inst(uart_inst)
    {
        // https://github.com/raspberrypi/pico-examples/blob/master/uart/uart_advanced/uart_advanced.c

        uart_init(_uart_inst, baudrate);
        
        gpio_set_function(tx_pin, UART_FUNCSEL_NUM(_uart_inst, tx_pin));
        gpio_set_function(rx_pin, UART_FUNCSEL_NUM(_uart_inst, rx_pin));

        // Set UART flow control CTS/RTS, we don't want these, so turn them off (each would add one more required wire)
        uart_set_hw_flow(_uart_inst, false, false);

        // Set our data format
        uart_set_format(_uart_inst, DATA_BITS, STOP_BITS, PARITY);

        // Turn off FIFO's - we want to do this character by character (default true)
        // uart_set_fifo_enabled(UART_ID, false);
    }

    void set_rx_interrupt(irq_handler_t on_uart_rx)
    {
        // Set up a RX interrupt
        // We need to set up the handler first
        // Select correct interrupt for the UART we are using
        const uint UART_IRQ = (_uart_inst == uart0 ? UART0_IRQ : UART1_IRQ);

        // And set up and enable the interrupt handlers
        irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
        irq_set_enabled(UART_IRQ, true);

        // Now enable the UART to send interrupts - RX only
        // void uart_set_irqs_enabled(uart_inst_t *uart, bool rx_has_data, bool tx_needs_data)
        uart_set_irqs_enabled(_uart_inst, true, false);
    }

    bool send_message(const char *msg)
    {
        if (!uart_is_writable(_uart_inst)) return false;
        uart_puts(_uart_inst, msg);
        return true;
    }
}

void comm_uart_interrupt_callback_wrapper(CommAPI_UART *api)
{
    while (uart_is_readable(api->_uart_inst))
    {
        uint8_t ch = uart_getc(api->_uart_inst);

        api->_incoming_buffer[api->_incoming_buffer_curr_index++] = ch;
        
        if (ch = api->_end_char || api->_incoming_buffer_curr_index >= COMM_API_READ_BUFFER-2)
        {
            api->_incoming_buffer[COMM_API_READ_BUFFER-1] = '\0';
            api->save_new_message(api->_incoming_buffer);
            api->_incoming_buffer_curr_index = 0;
        }
    }
}

/* Example of how to set up the CommAPI_UART with rx interrupts
 *
 * CommAPI_UART api(uart0, 115200, 0, 1, '\0', ' ');
 * void comm_uart_interrupt_callback()
 * {
 *     comm_uart_interrupt_callback_wrapper(&api);
 * }
 * ...
 * void main()
 * {
 *     api.set_rx_interrupt(comm_uart_interrupt_callback);
 * }
 * 
*/

#endif /* COMM_UART_HPP */