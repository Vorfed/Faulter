cmake_minimum_required(VERSION 3.16)

project(FaulterControlSuite LANGUAGES C CXX)

option(BUILD_HOST     "Build desktop host GUI application" ON)
option(BUILD_TESTS    "Build unit test suite"              ON)
option(BUILD_FIRMWARE "Build RP2040 Pico firmware"         OFF)

# 1. Desktop Host Application (Qt6 / Qt5)
if(BUILD_HOST)
    add_subdirectory(host-app)
endif()

# 2. Protocol & Logic Unit Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 3. RP2040 Firmware (requires Pico SDK)
if(BUILD_FIRMWARE)
    if(NOT DEFINED PICO_SDK_PATH AND NOT DEFINED ENV{PICO_SDK_PATH})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/firmware/lib/pico-sdk/pico_sdk_init.cmake")
            set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/firmware/lib/pico-sdk")
        endif()
    endif()
    add_subdirectory(firmware)
endif()
