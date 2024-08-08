add_executable(${PROJECT_NAME}
    main.cpp
)

# pull in common dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib
    # bme280
    # stepper
    # hx711_mux
    # analog_sensor

    # FreeRTOS-Kernel
    # FreeRTOS-Kernel-Heap4
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)