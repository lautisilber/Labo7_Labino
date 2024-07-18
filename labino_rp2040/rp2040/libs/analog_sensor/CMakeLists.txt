add_library(analog_sensor STATIC
analog_sensor.cpp analog_sensor.hpp
)
target_link_libraries(
    analog_sensor
    pico_stdlib
    hardware_adc
    hardware_dma
    panic
    utils
)
target_include_directories(analog_sensor PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
