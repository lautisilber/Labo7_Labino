set(fatfs_spi_lib_name fatfs_spi)
add_library(${fatfs_spi_lib_name} INTERFACE)
target_sources(${fatfs_spi_lib_name} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ff15/source/ffsystem.c
    ${CMAKE_CURRENT_LIST_DIR}/ff15/source/ffunicode.c
    ${CMAKE_CURRENT_LIST_DIR}/ff15/source/ff.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/sd_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/demo_logging.c
#    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/hw_config.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/spi.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/sd_card.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/crc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/glue.c
    ${CMAKE_CURRENT_LIST_DIR}/src/f_util.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ff_stdio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/rtc.c
)
target_include_directories(${fatfs_spi_lib_name} INTERFACE
    ff15/source
    sd_driver
    include
)
target_link_libraries(${fatfs_spi_lib_name} INTERFACE
        hardware_spi
        hardware_dma
        hardware_rtc
        pico_stdlib
)
