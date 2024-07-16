add_library(hx711_mux STATIC
hx711_mux.cpp hx711_mux.hpp
)
target_link_libraries(
    hx711_mux
    pico_stdlib
    hardware_gpio
    pico_time
    hx711_driver
    timer_interrupts_helper
    utils
)
target_include_directories(hx711_mux PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
