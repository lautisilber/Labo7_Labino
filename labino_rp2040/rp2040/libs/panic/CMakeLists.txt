add_library(panic STATIC
panic.c panic.h
)
target_link_libraries(
    panic
    pico_stdlib
    hardware_watchdog
)
target_include_directories(panic PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
