add_library(comm_api STATIC
    comm_api.cpp comm_api.hpp
    comm_uart.hpp
)

target_link_libraries(
    comm_api
    pico_stdlib
    pico_uart
    debug_helper
    utils
    FreeRTOS-Kernel
)

# target_link_libraries(
#     debug_helper
#     FreeRTOS
# )
#     FreeRTOS-Kernel
#     FreeRTOS-Static
# )
# if(FREE_RTOS_DYNAMIC)
# target_link_libraries(
#     user_flash
#     FreeRTOS-Kernel-Heap4
# )
# endif()
target_include_directories(comm_api PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
