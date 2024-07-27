
add_library(simple_pwm STATIC
    simple_pwm.cpp simple_pwm.hpp
)
target_link_libraries(
    simple_pwm
    pico_stdlib
    hardware_pwm
)
target_include_directories(simple_pwm PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
