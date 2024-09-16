add_subdirectory(bme280)
add_subdirectory(i2c_helper)
add_subdirectory(stepper)
add_subdirectory(hx711_driver)
add_subdirectory(hx711_mux)
add_subdirectory(utils)
add_subdirectory(user_flash)
add_subdirectory(user_panic)
add_subdirectory(analog_sensor)
add_subdirectory(debug_helper)
add_subdirectory(simple_pwm)
add_subdirectory(servo)
add_subdirectory(comm_api)
add_subdirectory(FreeRTOS-Static)
add_subdirectory(neopixel)
# add_subdirectory(fatfs_spi)

# add freertos

# Add FreeRTOS as a library

set(FREERTOS_CFG_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/FreeRTOS-Config")
set(FREERTOS_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/FreeRTOS-Kernel")

# this doesn't work for some reason
# # set(FREERTOS_PORT GCC_RP2040)
# set(FREERTOS_PORT GCC_ARM_CM0)
set(FREERTOS_KERNEL_PATH ${FREERTOS_SRC_DIRECTORY})
set(FREERTOS_CONFIG_FILE_DIRECTORY "${FREERTOS_CFG_DIRECTORY}")
include(FreeRTOS_Kernel_import.cmake)

# add_library(FreeRTOS-Kernel STATIC
#     "${FREERTOS_SRC_DIRECTORY}/croutine.c"
#     "${FREERTOS_SRC_DIRECTORY}/event_groups.c"
#     "${FREERTOS_SRC_DIRECTORY}/list.c"
#     "${FREERTOS_SRC_DIRECTORY}/queue.c"
#     "${FREERTOS_SRC_DIRECTORY}/stream_buffer.c"
#     "${FREERTOS_SRC_DIRECTORY}/tasks.c"
#     "${FREERTOS_SRC_DIRECTORY}/timers.c"
#     # "${FREERTOS_SRC_DIRECTORY}/portable/MemMang/heap_4.c"
#     "${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0/port.c"
#     "${FREERTOS_CFG_DIRECTORY}/FreeRTOSStaticHelper.c"
# )
# # Build FreeRTOS
# target_include_directories(FreeRTOS-Kernel PUBLIC
#     "${FREERTOS_CFG_DIRECTORY}/"
#     "${FREERTOS_SRC_DIRECTORY}/include"
#     "${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0"
# )
