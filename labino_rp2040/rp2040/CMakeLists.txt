cmake_minimum_required(VERSION 3.12)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

set(PROJECT_NAME StomaSenseRP2040)

project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_compile_definitions(PICO_DEFAULT_LED_PIN=22)

add_subdirectory(libs)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

# pull in common dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib
    bme280
    # fatfs_spi
    stepper
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
