add_library(user_flash STATIC
user_flash.c user_flash.h user_flash_class.cpp user_flash_class.hpp
)
target_link_libraries(
    user_flash
    pico_stdlib
    hardware_flash
    hardware_sync
    user_panic
    FreeRTOS-Kernel
    FreeRTOS-Static
)
if(FREE_RTOS_DYNAMIC)
target_link_libraries(
    user_flash
    FreeRTOS-Kernel-Heap4
)
endif()


target_include_directories(user_flash PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
