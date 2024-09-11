add_library(comm_api STATIC
    comm_api.hpp
)
target_link_libraries(
    debug_helper
    FreeRTOS
)
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
