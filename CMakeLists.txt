cmake_minimum_required(VERSION 3.16)
project(deepin-iman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql Concurrent Test)
find_package(Dtk6Widget REQUIRED)
find_package(Dtk6Core REQUIRED)

enable_testing()

# Sources added incrementally as tasks progress
set(APP_SOURCES
    src/main.cpp
)

add_executable(deepin-iman ${APP_SOURCES})

target_include_directories(deepin-iman PRIVATE
    src
    src/data
    src/service
    src/view
)

target_link_libraries(deepin-iman PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
    Dtk6::Widget
    Dtk6::Core
)

install(TARGETS deepin-iman DESTINATION bin)
