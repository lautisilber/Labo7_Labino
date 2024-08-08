add_executable(freertos_dynamic_test
freertos_dynamic_test.cpp
)

# pull in common dependencies
target_link_libraries(freertos_dynamic_test PUBLIC
    pico_stdlib
    hardware_gpio
    debug_helper
    FreeRTOS-Kernel
    FreeRTOS-Static
)
if(FREE_RTOS_DYNAMIC)
target_link_libraries(
    freertos_dynamic_test PUBLIC
    FreeRTOS-Kernel-Heap4
)
endif()

pico_add_extra_outputs(freertos_dynamic_test)

pico_enable_stdio_usb(freertos_dynamic_test 1)
pico_enable_stdio_uart(freertos_dynamic_test 0)