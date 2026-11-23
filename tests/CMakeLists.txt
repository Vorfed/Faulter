cmake_minimum_required(VERSION 3.16)

project(faulter_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Core Gui QUIET)
if(Qt6_FOUND)
    set(QT_LIBS Qt6::Widgets Qt6::Core Qt6::Gui)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui)
    set(QT_LIBS Qt5::Widgets Qt5::Core Qt5::Gui)
endif()

# 1. Headless Protocol Unit Tests
add_executable(run_tests
    test_protocol.cpp
    ../host-app/src/protocol/cobs.cpp
    ../host-app/src/protocol/crc16.cpp
    ../host-app/src/model/param_validator.cpp
    ../firmware/src/protocol/cobs.c
    ../firmware/src/protocol/crc16.c
    ../firmware/src/protocol/protocol.c
)

target_include_directories(run_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../shared
    ${CMAKE_CURRENT_LIST_DIR}/../host-app/src
    ${CMAKE_CURRENT_LIST_DIR}/../firmware/src
)

target_link_libraries(run_tests PRIVATE
    ${QT_LIBS}
)

# 2. GUI Component & Snapshot Test
add_executable(test_gui
    test_gui.cpp
    ../host-app/src/protocol/cobs.cpp
    ../host-app/src/protocol/crc16.cpp
    ../host-app/src/protocol/host_protocol.cpp
    ../host-app/src/serial/serial_backend.cpp
    ../host-app/src/model/fault_config.cpp
    ../host-app/src/model/param_validator.cpp
    ../host-app/src/ui/dark_theme.cpp
    ../host-app/src/ui/telemetry_console.cpp
    ../host-app/src/ui/warning_banner.cpp
    ../host-app/src/ui/config_panel.cpp
    ../host-app/src/ui/main_window.cpp
    ../host-app/assets/faulter_resources.qrc
)

target_include_directories(test_gui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../shared
    ${CMAKE_CURRENT_LIST_DIR}/../host-app/src
    ${CMAKE_CURRENT_LIST_DIR}/../firmware/src
)

target_link_libraries(test_gui PRIVATE
    ${QT_LIBS}
)

enable_testing()
add_test(NAME faulter_unit_tests COMMAND run_tests)
add_test(NAME faulter_gui_tests COMMAND test_gui)
