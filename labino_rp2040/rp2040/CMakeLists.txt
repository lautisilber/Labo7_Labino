cmake_minimum_required(VERSION 3.14)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set env variable 'PICO_SDK_PATH' to the local Pico SDK
# Comment out the set() if you have a global copy of the
# SDK set and $PICO_SDK_PATH defined in your $PATH
# set(ENV{PICO_SDK_PATH} "${CMAKE_SOURCE_DIR}/pico-sdk")

set(PROJECT_NAME StomaSenseRP2040)

project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise the Pico SDK
pico_sdk_init()

add_compile_definitions(PICO_DEFAULT_LED_PIN=22)

add_subdirectory(libs)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

add_executable(${PROJECT_NAME}
    src/main.cpp
)

# pull in common dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib
    bme280
    stepper
    hx711_mux
    analog_sensor
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
