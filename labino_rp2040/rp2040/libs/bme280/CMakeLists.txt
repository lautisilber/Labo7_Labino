add_library(bme280 STATIC
    bme280.c bme280.h
    bme280_class.cpp bme280_class.hpp
)
target_link_libraries(
    bme280
    i2c_helper
    hardware_i2c
    pico_stdlib
    hardware_sync
    utils
    debug_helper
    FreeRTOS
)
target_include_directories(bme280 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
