add_executable(freertos_comm_api
    freertos_comm_api.cpp
)

# pull in common dependencies
target_link_libraries(freertos_comm_api PUBLIC
    pico_stdlib
    debug_helper
    comm_api
    FreeRTOS-Kernel
    FreeRTOS-Static
)
# if(FREE_RTOS_DYNAMIC)
# target_link_libraries(
#     freertos_comm_api PUBLIC
#     FreeRTOS-Kernel-Heap4
# )
# endif()

pico_add_extra_outputs(freertos_comm_api)

pico_enable_stdio_usb(freertos_comm_api 1)
pico_enable_stdio_uart(freertos_comm_api 0)