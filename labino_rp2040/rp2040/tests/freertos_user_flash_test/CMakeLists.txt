add_executable(freertos_user_flash_test
freertos_user_flash_test.cpp
)

# pull in common dependencies
target_link_libraries(freertos_user_flash_test PUBLIC
    pico_stdlib
    hardware_gpio
    debug_helper
    user_flash
    # FreeRTOS-Kernel
    FreeRTOS-Static
)

pico_add_extra_outputs(freertos_user_flash_test)

pico_enable_stdio_usb(freertos_user_flash_test 1)
pico_enable_stdio_uart(freertos_user_flash_test 0)