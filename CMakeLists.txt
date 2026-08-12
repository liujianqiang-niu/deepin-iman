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
    src/data/SettingsStore.cpp
    src/data/ManIndex.cpp
    src/service/ManService.cpp
    src/service/SearchService.cpp
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

# Tests (added incrementally per task)
# Manually generate moc for test cpp file (Q_OBJECT class defined in .cpp)
get_target_property(MOC_EXECUTABLE Qt6::moc LOCATION)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TestSettingsStore.moc
    COMMAND ${MOC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestSettingsStore.cpp
            -o ${CMAKE_CURRENT_BINARY_DIR}/TestSettingsStore.moc
    DEPENDS tests/TestSettingsStore.cpp
    VERBATIM
)
add_executable(TestSettingsStore tests/TestSettingsStore.cpp src/data/SettingsStore.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/TestSettingsStore.moc)
target_link_libraries(TestSettingsStore PRIVATE Qt6::Core Qt6::Test)
target_include_directories(TestSettingsStore PRIVATE src src/data ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME TestSettingsStore COMMAND TestSettingsStore)

# TestManIndex
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TestManIndex.moc
    COMMAND ${MOC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestManIndex.cpp
            -o ${CMAKE_CURRENT_BINARY_DIR}/TestManIndex.moc
    DEPENDS tests/TestManIndex.cpp
    VERBATIM
)
add_executable(TestManIndex tests/TestManIndex.cpp src/data/ManIndex.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/TestManIndex.moc)
target_link_libraries(TestManIndex PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
target_include_directories(TestManIndex PRIVATE src src/data ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME TestManIndex COMMAND TestManIndex)

# TestManService
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TestManService.moc
    COMMAND ${MOC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestManService.cpp
            -o ${CMAKE_CURRENT_BINARY_DIR}/TestManService.moc
    DEPENDS tests/TestManService.cpp
    VERBATIM
)
add_executable(TestManService tests/TestManService.cpp src/service/ManService.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/TestManService.moc)
target_link_libraries(TestManService PRIVATE Qt6::Core Qt6::Concurrent Qt6::Test)
target_include_directories(TestManService PRIVATE src src/service ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME TestManService COMMAND TestManService)

# TestSearchService
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TestSearchService.moc
    COMMAND ${MOC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestSearchService.cpp
            -o ${CMAKE_CURRENT_BINARY_DIR}/TestSearchService.moc
    DEPENDS tests/TestSearchService.cpp
    VERBATIM
)
add_executable(TestSearchService tests/TestSearchService.cpp src/data/ManIndex.cpp src/service/SearchService.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/TestSearchService.moc)
target_link_libraries(TestSearchService PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
target_include_directories(TestSearchService PRIVATE src src/data src/service ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME TestSearchService COMMAND TestSearchService)
