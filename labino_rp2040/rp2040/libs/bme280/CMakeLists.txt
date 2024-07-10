set(BME_280_lib_name bme280)
add_library(${BME_280_lib_name} STATIC
    bme280.c bme280.h
    bme280_i2c.c bme280_i2c.h
    bme280_handler.c bme280_handler.h
    bme280_class.hpp
)
target_link_libraries(
    ${BME_280_lib_name}
    hardware_i2c
    pico_stdlib
)
target_include_directories(${BME_280_lib_name} PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
