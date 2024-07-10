set(stepper_lib_name stepper)
add_library(${stepper_lib_name} STATIC
    stepper.cpp stepper.hpp
)
target_link_libraries(
    ${stepper_lib_name}
    pico_stdlib
    hardware_gpio
    pico_time
)
target_include_directories(${stepper_lib_name} PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
