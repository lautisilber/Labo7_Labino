add_library(hx711_mux STATIC
hx711_mux.cpp hx711_mux.hpp
)
target_link_libraries(
    hx711_mux
    pico_stdlib
    hardware_gpio
    pico_time
    hx711_driver
    hardware_sync
    utils
    user_flash
)
target_include_directories(hx711_mux PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
