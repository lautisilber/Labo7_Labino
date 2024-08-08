# add_library(FreeRTOS-Static STATIC
# FreeRTOS_Static.c FreeRTOS_Static.h
# )
# target_link_libraries(
#     FreeRTOS-Static
#     FreeRTOS-Kernel
#     # FreeRTOS-Kernel-Heap4
# )
# target_include_directories(FreeRTOS-Static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(FreeRTOS-Static INTERFACE)
target_link_libraries(
    FreeRTOS-Static INTERFACE
    FreeRTOS-Kernel
)
if(FREE_RTOS_DYNAMIC)
target_link_libraries(
    FreeRTOS-Static INTERFACE
    FreeRTOS-Kernel-Heap4
)
endif()
target_include_directories(FreeRTOS-Static INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})