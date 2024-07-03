set(BME_280_i2c_lib_name bme280_i2c)
add_library(${BME_280_i2c_lib_name} STATIC bme280_i2c.c bme280_i2c.h)
target_link_libraries(
    ${BME_280_i2c_lib_name}
    bme280
    pico_stdlib
    hardware_i2c
)
target_include_directories(${BME_280_i2c_lib_name} PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
