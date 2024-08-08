add_library(user_panic INTERFACE)
target_link_libraries(
    user_panic INTERFACE
    hardware_watchdog
    debug_helper
)
target_include_directories(user_panic INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
