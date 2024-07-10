set(BME_280_handler_lib_name bme280_handler)
add_library(${BME_280_handler_lib_name} STATIC
    bme280_i2c.c bme280_i2c.h
    bme280_handler.c bme280_handler.h
)
target_link_libraries(
    ${BME_280_handler_lib_name}
    pico_stdlib
    hardware_i2c
    bme280
)
target_include_directories(${BME_280_handler_lib_name} PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
