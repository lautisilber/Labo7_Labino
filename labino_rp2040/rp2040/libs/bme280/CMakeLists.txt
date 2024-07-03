set(BME_280_lib_name bme280)
add_library(${BME_280_lib_name} STATIC bme280.c bme280.h)
target_include_directories(${BME_280_lib_name} PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
