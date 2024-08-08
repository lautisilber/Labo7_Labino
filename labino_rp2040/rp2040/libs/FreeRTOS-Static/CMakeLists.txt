add_library(FreeRTOS-Static STATIC
FreeRTOS_Static.c FreeRTOS_Static.h
)
target_link_libraries(
    FreeRTOS-Static
    FreeRTOS-Kernel
    # FreeRTOS-Kernel-Heap4
)
target_include_directories(FreeRTOS-Static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
