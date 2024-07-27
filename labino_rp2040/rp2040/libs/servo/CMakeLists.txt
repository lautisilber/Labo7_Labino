
add_library(servo STATIC
    servo.cpp servo.hpp
)
target_link_libraries(
    servo
    pico_stdlib
    debug_helper
    utils
    simple_pwm
    pico_time
)
target_include_directories(servo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
