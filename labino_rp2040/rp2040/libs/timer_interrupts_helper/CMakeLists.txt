add_library(timer_interrupts_helper STATIC
timer_interrupts_helper.c timer_interrupts_helper.h
)
target_link_libraries(
    timer_interrupts_helper
    pico_stdlib
    hardware_irq
)
target_include_directories(timer_interrupts_helper PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
