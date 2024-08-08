add_executable(freertos_hw_timer_test
freertos_hw_timer_test.cpp
)

# pull in common dependencies
target_link_libraries(freertos_hw_timer_test PUBLIC
    pico_stdlib
    hardware_gpio
    debug_helper
    pico_time
    FreeRTOS-Kernel
    FreeRTOS-Static
)

pico_add_extra_outputs(freertos_hw_timer_test)

pico_enable_stdio_usb(freertos_hw_timer_test 1)
pico_enable_stdio_uart(freertos_hw_timer_test 0)