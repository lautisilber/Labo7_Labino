add_executable(panic_test
panic_test.cpp
)

# pull in common dependencies
target_link_libraries(panic_test PUBLIC
    pico_stdlib
    debug_helper
    user_panic
)

pico_add_extra_outputs(panic_test)

pico_enable_stdio_usb(panic_test 1)
pico_enable_stdio_uart(panic_test 0)