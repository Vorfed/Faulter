cmake_minimum_required(VERSION 3.16)

project(faulter_host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 with fallback to Qt5
find_package(Qt6 COMPONENTS Widgets Core Gui QUIET)
if(Qt6_FOUND)
    message(STATUS "Faulter Host: Using Qt6 (${Qt6_VERSION})")
    set(QT_LIBRARIES Qt6::Widgets Qt6::Core Qt6::Gui)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui)
    message(STATUS "Faulter Host: Using Qt5 (${Qt5_VERSION})")
    set(QT_LIBRARIES Qt5::Widgets Qt5::Core Qt5::Gui)
endif()

set(FAULTER_HOST_SOURCES
    src/main.cpp
    src/protocol/cobs.cpp
    src/protocol/crc16.cpp
    src/protocol/host_protocol.cpp
    src/serial/serial_backend.cpp
    src/model/fault_config.cpp
    src/model/param_validator.cpp
    src/ui/dark_theme.cpp
    src/ui/telemetry_console.cpp
    src/ui/warning_banner.cpp
    src/ui/config_panel.cpp
    src/ui/main_window.cpp
    assets/faulter_resources.qrc
)

add_executable(faulter ${FAULTER_HOST_SOURCES})

target_include_directories(faulter PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../shared
)

target_link_libraries(faulter PRIVATE
    ${QT_LIBRARIES}
)
