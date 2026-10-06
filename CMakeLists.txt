cmake_minimum_required(VERSION 3.16)
project(nitro-control VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

qt_add_executable(nitro-control)

# Layered layout: core = engine (no UI), ui = widgets/windows, app = entry.
target_sources(nitro-control PRIVATE
    src/app/main.cpp
    src/app/selftest.cpp
    src/core/config.h
    src/core/sensors.h    src/core/sensors.cpp
    src/core/control.h    src/core/control.cpp
    src/ui/theme.h        src/ui/theme.cpp
    src/ui/icon.h         src/ui/icon.cpp
    src/ui/segmented.h    src/ui/segmented.cpp
    src/ui/popup.h        src/ui/popup.cpp
    src/ui/rgbpanel.h     src/ui/rgbpanel.cpp
    src/ui/tray.h         src/ui/tray.cpp
)

target_include_directories(nitro-control PRIVATE src)

target_link_libraries(nitro-control PRIVATE Qt6::Widgets Qt6::Concurrent)
install(TARGETS nitro-control RUNTIME DESTINATION bin)
