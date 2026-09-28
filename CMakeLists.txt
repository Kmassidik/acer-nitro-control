cmake_minimum_required(VERSION 3.16)
project(nitro-control VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(nitro-control
    src/main.cpp
    src/config.h
    src/sensors.h   src/sensors.cpp
    src/control.h   src/control.cpp
    src/theme.h     src/theme.cpp
    src/icon.h      src/icon.cpp
    src/levelbutton.h src/levelbutton.cpp
    src/popup.h     src/popup.cpp
    src/rgbpanel.h  src/rgbpanel.cpp
    src/tray.h      src/tray.cpp
)

target_link_libraries(nitro-control PRIVATE Qt6::Widgets)
install(TARGETS nitro-control RUNTIME DESTINATION bin)
