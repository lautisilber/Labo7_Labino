add_executable(neopixel_test
neopixel_test.cpp
)

# pull in common dependencies
target_link_libraries(neopixel_test PUBLIC
    pico_stdlib
    neopixel
)

pico_add_extra_outputs(neopixel_test)

pico_enable_stdio_usb(neopixel_test 1)
pico_enable_stdio_uart(neopixel_test 0)