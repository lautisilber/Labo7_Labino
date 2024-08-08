cmake_minimum_required(VERSION 3.14)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set env variable 'PICO_SDK_PATH' to the local Pico SDK
# Comment out the set() if you have a global copy of the
# SDK set and $PICO_SDK_PATH defined in your $PATH
# set(ENV{PICO_SDK_PATH} "${CMAKE_SOURCE_DIR}/pico-sdk")

set(PROJECT_NAME StomaSenseRP2040)

project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(VERSION_NUMBER "1.0.0")
set(BUILD_NUMBER "1")

# Initialise the Pico SDK
pico_sdk_init()

add_compile_definitions(PICO_DEFAULT_LED_PIN=22)

set(FREE_RTOS_STATIC 1) # should be 1 or 0
add_definitions( -DFREE_RTOS_STATIC=${FREE_RTOS_STATIC} )
set(FREE_RTOS_DYNAMIC 1) # should be 1 or 0
add_definitions( -DFREE_RTOS_DYNAMIC=${FREE_RTOS_DYNAMIC} )

add_subdirectory(libs)

# add src
# add_subdirectory(src)

# add test
add_subdirectory(tests)


