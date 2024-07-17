add_library(interrupts_helper INTERFACE)
# target_link_libraries(
#     interrupts_helper
#     hardware_sync
# )
target_include_directories(
    interrupts_helper
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)