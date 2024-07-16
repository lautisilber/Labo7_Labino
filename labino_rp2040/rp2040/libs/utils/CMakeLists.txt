add_library(utils STATIC
utils.cpp utils.hpp
)
target_link_libraries(
    utils
    pico_stdlib
)
target_include_directories(utils PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
