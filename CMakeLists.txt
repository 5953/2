cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(stm8prog C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# ---- FatFs (第三方源码, 不随仓库分发) ----
# 本地构建: 下载 https://elm-chan.org/fsw/ff/arc/ff15.zip
#           把压缩包里 source/ 下的 *.c *.h 复制到 lib/fatfs/
# GitHub Actions 会自动完成这一步.
file(GLOB FATFS_SOURCES CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_LIST_DIR}/lib/fatfs/source/*.c
     ${CMAKE_CURRENT_LIST_DIR}/lib/fatfs/*.c)
if (NOT FATFS_SOURCES)
    message(FATAL_ERROR
        "未找到 FatFs 源码! 请下载 ff15.zip 并把 source/ 里的文件复制到 lib/fatfs/")
endif()

add_executable(stm8prog
    src/main.c
    src/ui.c
    src/oled.c
    src/keys.c
    src/storage.c
    src/msc_disk.c
    src/fatfs_disk.c
    src/files.c
    src/hexparse.c
    src/swim.c
    src/stm8.c
    src/usb_descriptors.c
    ${FATFS_SOURCES}
)

target_include_directories(stm8prog PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/lib/fatfs
    ${CMAKE_CURRENT_LIST_DIR}/lib/fatfs/source
)

target_link_libraries(stm8prog PRIVATE
    pico_stdlib
    hardware_i2c
    hardware_flash
    hardware_sync
    pico_unique_id
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_uart(stm8prog 0)
pico_enable_stdio_usb(stm8prog 0)

pico_add_extra_outputs(stm8prog)