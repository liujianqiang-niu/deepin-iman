cmake_minimum_required(VERSION 3.16)
project(deepin-iman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql Concurrent Test LinguistTools)
find_package(Dtk6Widget REQUIRED)
find_package(Dtk6Core REQUIRED)
find_program(MANDOC_EXECUTABLE mandoc REQUIRED)

enable_testing()

# Sources
set(APP_SOURCES
    src/main.cpp
    src/data/SettingsStore.cpp
    src/data/ManIndex.cpp
    src/data/TranslationCache.cpp
    src/data/FavoriteDb.cpp
    src/data/HistoryDb.cpp
    src/service/ManService.cpp
    src/service/SearchService.cpp
    src/service/TranslationService.cpp
    src/service/ExampleService.cpp
    src/service/FavoriteService.cpp
    src/service/HistoryService.cpp
    src/service/ai/AiService.cpp
    src/service/ai/IAiProvider.cpp
    src/service/ai/OpenAiCompatibleProvider.cpp
    src/view/MainWindow.cpp
    src/view/LeftSidebar.cpp
    src/view/ManView.cpp
    src/view/AiChatWidget.cpp
    src/view/SettingsDialog.cpp
    src/view/TranslationView.cpp
    src/view/ResultViewDialog.cpp
    src/view/SplitViewDialog.cpp
    src/view/PageListDialog.cpp
)

add_executable(deepin-iman ${APP_SOURCES} resources.qrc)

target_include_directories(deepin-iman PRIVATE
    src
    src/data
    src/service
    src/service/ai
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

# Install prompt templates
install(DIRECTORY data/prompts DESTINATION share/deepin-iman)

install(TARGETS deepin-iman DESTINATION bin)

# Install application icon (SVG scalable + PNG sizes)
install(FILES assets/icons/deepin-iman.svg
        DESTINATION share/icons/hicolor/scalable/apps)
foreach(size 16 22 24 32 48 64 128 256)
    install(FILES assets/icons/deepin-iman-${size}.png
            DESTINATION share/icons/hicolor/${size}x${size}/apps
            RENAME deepin-iman.png)
endforeach()

# Install desktop file
install(FILES debian/deepin-iman.desktop DESTINATION share/applications)

# Install man page
install(FILES debian/deepin-iman.1 DESTINATION share/man/man1)

# Translations
file(GLOB TS_FILES translations/*.ts)
set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/translations)
find_program(LRELEASE_EXECUTABLE lrelease PATHS /usr/lib/qt6/bin /usr/lib/qt5/bin NO_DEFAULT_PATH)
set(QM_FILES "")
foreach(ts_file ${TS_FILES})
    get_filename_component(qm_name ${ts_file} NAME_WE)
    set(qm_file ${CMAKE_CURRENT_BINARY_DIR}/translations/${qm_name}.qm)
    add_custom_command(
        OUTPUT ${qm_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/translations
        COMMAND ${LRELEASE_EXECUTABLE} ${ts_file} -qm ${qm_file}
        DEPENDS ${ts_file}
        VERBATIM
    )
    list(APPEND QM_FILES ${qm_file})
endforeach()
add_custom_target(translations ALL DEPENDS ${QM_FILES})
add_dependencies(deepin-iman translations)
install(FILES ${QM_FILES} DESTINATION share/deepin-iman/translations)

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

# TestIntegration
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TestIntegration.moc
    COMMAND ${MOC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestIntegration.cpp
            -o ${CMAKE_CURRENT_BINARY_DIR}/TestIntegration.moc
    DEPENDS tests/TestIntegration.cpp
    VERBATIM
)
add_executable(TestIntegration tests/TestIntegration.cpp
    src/data/ManIndex.cpp src/service/ManService.cpp src/service/SearchService.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/TestIntegration.moc)
target_link_libraries(TestIntegration PRIVATE Qt6::Core Qt6::Sql Qt6::Concurrent Qt6::Test)
target_include_directories(TestIntegration PRIVATE src src/data src/service ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME TestIntegration COMMAND TestIntegration)

# TestAiProvider
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TestAiProvider.moc
    COMMAND ${MOC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestAiProvider.cpp
            -o ${CMAKE_CURRENT_BINARY_DIR}/TestAiProvider.moc
    DEPENDS tests/TestAiProvider.cpp
    VERBATIM
)
add_executable(TestAiProvider tests/TestAiProvider.cpp
    src/service/ai/OpenAiCompatibleProvider.cpp
    src/service/ai/IAiProvider.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/TestAiProvider.moc)
target_link_libraries(TestAiProvider PRIVATE Qt6::Core Qt6::Network Qt6::Test)
target_include_directories(TestAiProvider PRIVATE src src/service/ai ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME TestAiProvider COMMAND TestAiProvider)
