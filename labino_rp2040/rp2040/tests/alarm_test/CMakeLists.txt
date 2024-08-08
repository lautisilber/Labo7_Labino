add_executable(alarm_test
alarm_test.cpp
)

# pull in common dependencies
target_link_libraries(alarm_test PUBLIC
    pico_stdlib
    debug_helper
    hardware_sync
)

pico_add_extra_outputs(alarm_test)

pico_enable_stdio_usb(alarm_test 1)
pico_enable_stdio_uart(alarm_test 0)