add_executable(flash_test_raw
flash_test_raw.cpp
)

# pull in common dependencies
target_link_libraries(flash_test_raw PUBLIC
    pico_stdlib
    hardware_flash
    hardware_sync
)

pico_add_extra_outputs(flash_test_raw)

pico_enable_stdio_usb(flash_test_raw 1)
pico_enable_stdio_uart(flash_test_raw 0)