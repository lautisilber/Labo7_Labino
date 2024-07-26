add_library(hx711_driver STATIC
    hx711_driver.c hx711_driver.h
)
target_link_libraries(
    hx711_driver
    pico_stdlib
    hardware_gpio
    pico_time
)
target_include_directories(hx711_driver PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
