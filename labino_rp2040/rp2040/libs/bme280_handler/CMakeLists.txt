set(BME_280_handler_lib_name bme280_handler)
add_library(${BME_280_handler_lib_name} STATIC bme280_handler.c bme280_handler.h)
target_link_libraries(
    ${BME_280_handler_lib_name}
    bme280_i2c
    pico_stdlib
    hardware_i2c
)
target_include_directories(${BME_280_handler_lib_name} PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
