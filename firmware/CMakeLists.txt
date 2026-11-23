cmake_minimum_required(VERSION 3.13)

# Auto-detect bundled pico-sdk submodule if PICO_SDK_PATH not set
if(NOT DEFINED PICO_SDK_PATH AND NOT DEFINED ENV{PICO_SDK_PATH})
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk/pico_sdk_init.cmake")
        set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/pico-sdk")
    endif()
endif()

# Pull in Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

project(faulter_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize Pico SDK
pico_sdk_init()

add_executable(faulter_fw
    src/main.c
    src/protocol/cobs.c
    src/protocol/crc16.c
    src/protocol/protocol.c
    src/glitch/glitch_engine.c
    src/power/power_cycle.c
    src/state/state_machine.c
)

# Generate header from PIO assembly
pico_generate_pio_header(faulter_fw ${CMAKE_CURRENT_LIST_DIR}/src/glitch/glitch.pio)

target_include_directories(faulter_fw PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../shared
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(faulter_fw
    pico_stdlib
    hardware_pio
    hardware_gpio
    hardware_timer
)

# Enable USB CDC serial, disable UART to prevent output interference
pico_enable_stdio_usb(faulter_fw 1)
pico_enable_stdio_uart(faulter_fw 0)

# Generate UF2 / HEX / BIN / MAP outputs
pico_add_extra_outputs(faulter_fw)
