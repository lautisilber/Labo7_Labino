
add_library(stepper STATIC
    stepper.cpp stepper.hpp
)
target_link_libraries(
    stepper
    pico_stdlib
    hardware_gpio
    pico_time
    pico_multicore
    user_flash
    user_panic
)
target_include_directories(stepper PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
