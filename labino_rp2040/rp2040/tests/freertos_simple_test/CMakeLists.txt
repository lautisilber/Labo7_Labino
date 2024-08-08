add_executable(freertos_simple_test
freertos_simple_test.cpp
)

# pull in common dependencies
target_link_libraries(freertos_simple_test PUBLIC
    pico_stdlib
    hardware_gpio
    debug_helper
)
if(FREERTOS_STATIC)
target_link_libraries(
    freertos_simple_test PUBLIC
    FreeRTOS-Static
)
else()
target_link_libraries(
    freertos_simple_test PUBLIC
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)
endif()

pico_add_extra_outputs(freertos_simple_test)

pico_enable_stdio_usb(freertos_simple_test 1)
pico_enable_stdio_uart(freertos_simple_test 0)