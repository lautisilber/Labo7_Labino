set(fatfs_lib_name fatfs)
add_library(${fatfs_lib_name} STATIC
    diskio.c diskio.h
    ff.c ff.h
    ffconf.h
    ffsystem.c
    ffunicode.c
)

target_include_directories(${fatfs_lib_name} PUBLIC "${CMAKE_CURRENT_COURCE_DIR}")
