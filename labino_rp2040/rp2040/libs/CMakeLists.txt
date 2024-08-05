add_subdirectory(bme280)
add_subdirectory(i2c_helper)
add_subdirectory(stepper)
add_subdirectory(hx711_driver)
add_subdirectory(hx711_mux)
add_subdirectory(utils)
add_subdirectory(user_flash)
add_subdirectory(panic)
add_subdirectory(analog_sensor)
add_subdirectory(debug_helper)
add_subdirectory(simple_pwm)
add_subdirectory(servo)
# add_subdirectory(fatfs_spi)

# add freertos

# Add FreeRTOS as a library
set(FREERTOS_CFG_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/FreeRTOS-Config")
set(FREERTOS_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/FreeRTOS-Kernel")
add_library(FreeRTOS STATIC
    "${FREERTOS_SRC_DIRECTORY}/event_groups.c"
    # /Users/lautarosilbergleit/Documents/UBA/Labo 7/Codigo/Arduino/labino_rp2040/rp2040/libs/FreeRTOS-Kernel/event_groups.c
    "${FREERTOS_SRC_DIRECTORY}/list.c"
    "${FREERTOS_SRC_DIRECTORY}/queue.c"
    "${FREERTOS_SRC_DIRECTORY}/stream_buffer.c"
    "${FREERTOS_SRC_DIRECTORY}/tasks.c"
    "${FREERTOS_SRC_DIRECTORY}/timers.c"
    "${FREERTOS_SRC_DIRECTORY}/portable/MemMang/heap_3.c"
    "${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0/port.c"
)
# Build FreeRTOS
target_include_directories(FreeRTOS PUBLIC
    "${FREERTOS_CFG_DIRECTORY}/"
    "${FREERTOS_SRC_DIRECTORY}/include"
    "${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0"
)