add_library(i2c_helper STATIC
    i2c_helper.c i2c_helper.h
)
target_link_libraries(
    i2c_helper
    hardware_i2c
    pico_stdlib
)
target_include_directories(i2c_helper PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
