#ifndef COMM_API_HPP
#define COMM_API_HPP


#include "pico/stdlib.h"
#include <string.h>
#include <assert.h>
#include "debug_helper.h"
#include "utils.hpp"

#if __has_include("FreeRTOS.h")
#include "FreeRTOS.h"
#include "task.h"

#define COMM_API_FREERTOS_PROCESS_MSG_STACK 1024
#endif


#define COMM_API_READ_BUFFER       256
#define COMM_API_MAX_ARGUMENTS     16
#define COMM_API_FORCE_END_CHAR    true
#define COMM_API_MESSAGES_BUFFER   8
#define COMM_API_N_DIFF_MESSAGES   32 // cantidad de mensajes differentes para asociar con callbacks
#define COMM_API_DEFAULT_END_CHAR  ((char)'\n')
#define COMM_API_DEFAULT_SEP_CHAR  ((char)' ')

static_assert(COMM_API_MAX_ARGUMENTS >= 1);

/*
    Only ascii chars are allowed
*/

typedef void (*comm_callback_t)(const char **, size_t);
typedef void (*comm_default_callback_t)(const char *, const char **, size_t);

class CommAPI
{
private:
    class CommMsg
    {
    private:
        char _msg[COMM_API_READ_BUFFER] = {0};
        char *_arguments[COMM_API_MAX_ARGUMENTS] = {nullptr};
        size_t _n_arguments = 0;

    public:
        // the command is the first 'word' of the message
        // although internally the command is treated as the first argument, it is not exposed this way.
        // the command has its own class and argument 0 points to the internal argument 1
        size_t get_n_arguments() const { return (_n_arguments > 0 ? _n_arguments-1 : 0); }
        const char *get_argument(size_t n) const
        {
            if (n < _n_arguments-1) return nullptr;
            return (const char *)_arguments[n+1];
        }
        const char *get_command() const
        {
            return get_argument(0);
        }
        const char **get_all_arguments() const
        {
            if (get_n_arguments() == 0) return nullptr;
            return (const char **)_arguments[1];
        }
        bool set_raw_msg(const char *msg);
        void process_msg(char end_char, char sep_char);
    };

    struct CommCommandResponse
    {
        const char *command;
        comm_callback_t callback;
    };
public:
    // not to be used by user
    const char _end_char, _sep_char;
private:
    FIFOStackForClasses<CommMsg, COMM_API_MESSAGES_BUFFER> _msgs;

    comm_default_callback_t _default_response_callback;
    CommCommandResponse _responses[COMM_API_N_DIFF_MESSAGES];
    size_t _active_responses = 0;
public:
    // not to be used by user. to be used in the interrupt callback wrapper of each implementation
    char _incoming_buffer[COMM_API_READ_BUFFER] = {0};
    size_t _incoming_buffer_curr_index = 0;
#ifdef FREE_RTOS_INSTALLED
public:
    // not to be used by user. only for freertos message processing task creation
    struct MessageProcessingTaskData { CommAPI *api; TickType_t delay_ticks; };
    MessageProcessingTaskData message_processing_task_data;
    StackType_t message_processing_task_stack[COMM_API_FREERTOS_PROCESS_MSG_STACK];
    StaticTask_t message_processing_task_buffer;
#endif
public:
    CommAPI(char end_char, char sep_char) : _end_char(end_char), _sep_char(sep_char) {};
    bool add_response(const char * const command, comm_callback_t callback)
    {
        if (_active_responses >= COMM_API_N_DIFF_MESSAGES-1) return false;
        _responses[_active_responses].command = command;
        _responses[_active_responses].callback = callback;
        ++_active_responses;
        return true;
    }
    void add_default_response(comm_default_callback_t callback)
    {
        _default_response_callback = callback;
    }

    bool save_new_message(const char *msg)
    {
        if (_msgs.full()) return false;
        _msgs.add_slot_back();
        CommMsg *comm_msg = _msgs.peek_back_to_edit();
        return comm_msg->set_raw_msg(msg);
    }

    bool unprocessed_messages() const { return _msgs.empty(); }

    bool process_next_saved_message()
    {
        if (unprocessed_messages()) return false;

        CommMsg *msg = _msgs.peek_front_to_read();
        _msgs.remove_slot_front();
        msg->process_msg(_end_char, _sep_char);
        for (size_t i = 0; i < _active_responses; i++)
        {
            const CommCommandResponse *response = &_responses[i];
            if (strncmp(msg->get_command(), response->command, COMM_API_READ_BUFFER) == 0)
            {
                #ifdef FREE_RTOS_INSTALLED
                taskYIELD();
                #endif

                #ifdef D_DEBUG
                DEBUG_PRINTF("Processing message with command '%s' and arguments ", msg->get_command());
                if (msg->get_n_arguments() == 0)
                {
                    printf("\n");
                }
                else
                {
                    for (size_t _i = 0; _i < msg->get_n_arguments(); _i++)
                    {
                        printf("'%s'", msg->get_argument(_i));
                        if (_i < msg->get_n_arguments()-1)
                            printf(", ");
                        else
                            printf("\n");
                    }
                }
                #endif

                response->callback(msg->get_all_arguments(), msg->get_n_arguments());
                return true;
            }
        }

        #ifdef FREE_RTOS_INSTALLED
        taskYIELD();
        #endif

        #ifdef D_DEBUG
        DEBUG_PRINTFLN("Processing message with default callback. Command is '%s' and arguments are ", msg->get_command());
        if (msg->get_n_arguments() == 0)
        {
            printf("\n");
        }
        else
        {
            for (size_t _i = 0; _i < msg->get_n_arguments(); _i++)
            {
                printf("'%s'", msg->get_argument(_i));
                if (_i < msg->get_n_arguments()-1)
                    printf(", ");
                else
                    printf("\n");
            }
        }
        #endif

        _default_response_callback(msg->get_command(), msg->get_all_arguments(), msg->get_n_arguments());

        return true;
    }

    #ifdef FREE_RTOS_INSTALLED
    static void message_processing_task(void *pvParameters)
    {
        MessageProcessingTaskData *data = (MessageProcessingTaskData *)pvParameters;

        for (;;)
        {
            data->api->process_next_saved_message();
            vTaskDelay(data->delay_ticks);
        }
    }

    TaskHandle_t create_message_processing_task(UBaseType_t priority = 1, TickType_t delay_ticks = 5, const char *task_name = "msg_proc")
    {
        message_processing_task_data.api = this;
        message_processing_task_data.delay_ticks = delay_ticks;

        return xTaskCreateStatic(
            CommAPI::message_processing_task,
            task_name,
            COMM_API_FREERTOS_PROCESS_MSG_STACK,
            &message_processing_task_data,
            priority,
            message_processing_task_stack,
            &message_processing_task_buffer
        );
    }
    #endif

    bool process_all_saved_messages()
    {
        bool res = false;
        while (process_next_saved_message())
        {
            res = true;
            #ifdef FREE_RTOS_INSTALLED
            if (unprocessed_messages())
                taskYIELD();
            #endif
        }
        return res;
    }

    virtual bool send_message(const char *msg) = 0;
};


#endif /* COMM_API_HPP */