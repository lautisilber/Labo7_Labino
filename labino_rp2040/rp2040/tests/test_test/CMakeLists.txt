add_executable(test_test
test_test.cpp
)

# pull in common dependencies
target_link_libraries(test_test PUBLIC
    pico_stdlib
)

pico_add_extra_outputs(test_test)

pico_enable_stdio_usb(test_test 1)
pico_enable_stdio_uart(test_test 0)