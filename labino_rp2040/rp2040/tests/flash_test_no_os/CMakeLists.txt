add_executable(flash_test_no_os
flash_test_no_os.cpp
)

# pull in common dependencies
target_link_libraries(flash_test_no_os PUBLIC
    pico_stdlib
    user_flash
)

pico_add_extra_outputs(flash_test_no_os)

pico_enable_stdio_usb(flash_test_no_os 1)
pico_enable_stdio_uart(flash_test_no_os 0)