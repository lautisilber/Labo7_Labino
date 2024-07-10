set(i2c_helper_lib_name i2c_helper)
add_library(${i2c_helper_lib_name} STATIC
    i2c_helper.c i2c_helper.h
)
target_link_libraries(
    ${i2c_helper_lib_name}
    hardware_i2c
    pico_stdlib
)
target_include_directories(${i2c_helper_lib_name} PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
