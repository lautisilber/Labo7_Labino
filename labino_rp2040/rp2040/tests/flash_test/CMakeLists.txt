add_executable(flash_test
flash_test.cpp
)

# pull in common dependencies
target_link_libraries(flash_test PUBLIC
    pico_stdlib
    debug_helper
    user_flash
)

pico_add_extra_outputs(flash_test)

pico_enable_stdio_usb(flash_test 1)
pico_enable_stdio_uart(flash_test 0)