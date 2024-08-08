add_executable(seeedstudio_rp2040_test
seeedstudio_rp2040_test.cpp
)

# pull in common dependencies
target_link_libraries(seeedstudio_rp2040_test PUBLIC
    pico_stdlib
    debug_helper
    hardware_gpio
)

pico_add_extra_outputs(seeedstudio_rp2040_test)

pico_enable_stdio_usb(seeedstudio_rp2040_test 1)
pico_enable_stdio_uart(seeedstudio_rp2040_test 0)