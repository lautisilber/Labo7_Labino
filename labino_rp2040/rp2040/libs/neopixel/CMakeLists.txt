add_library(neopixel INTERFACE)

pico_generate_pio_header(neopixel ${CMAKE_CURRENT_LIST_DIR}/ws2812byte.pio)

target_sources(neopixel INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/NeoPixel.cpp
)

target_include_directories(neopixel INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

# Pull in pico libraries that we need
target_link_libraries(neopixel INTERFACE pico_stdlib hardware_pio pico_malloc pico_mem_ops)