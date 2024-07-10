set(stepper_lib_name stepper)
add_library(${stepper_lib_name} STATIC
    stepper_raw.c stepper_raw.h
    stepper.cpp stepper.hpp
)
target_link_libraries(
    ${stepper_lib_name}
    pico_stdlib
)
target_include_directories(${stepper_lib_name} PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
