# 简化版 pico_sdk_import.cmake
# 优先用环境变量 PICO_SDK_PATH; 没有则自动从 GitHub 拉取(含 tinyusb 子模块).
if (NOT PICO_SDK_PATH)
    if (DEFINED ENV{PICO_SDK_PATH})
        set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    endif ()
endif ()

if (NOT PICO_SDK_PATH)
    include(FetchContent)
    set(PICO_SDK_FETCH_FROM_GIT_TAG "2.1.1" CACHE STRING "pico-sdk 版本")
    message(STATUS "PICO_SDK_PATH 未设置, 自动拉取 pico-sdk ${PICO_SDK_FETCH_FROM_GIT_TAG} ...")
    FetchContent_Declare(
        pico_sdk
        GIT_REPOSITORY https://github.com/raspberrypi/pico-sdk.git
        GIT_TAG        ${PICO_SDK_FETCH_FROM_GIT_TAG}
        GIT_SUBMODULES lib/tinyusb
    )
    FetchContent_GetProperties(pico_sdk)
    if (NOT pico_sdk_POPULATED)
        FetchContent_Populate(pico_sdk)
    endif ()
    set(PICO_SDK_PATH ${pico_sdk_SOURCE_DIR})
endif ()

get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

if (NOT EXISTS ${PICO_SDK_PATH}/pico_sdk_init.cmake)
    message(FATAL_ERROR "目录 '${PICO_SDK_PATH}' 不是有效的 pico-sdk")
endif ()

set(PICO_SDK_PATH ${PICO_SDK_PATH} CACHE PATH "Path to the Raspberry Pi Pico SDK" FORCE)

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)