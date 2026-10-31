cmake_minimum_required(VERSION 3.16)

project(NiumaV2SourcePreview VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Test Sql Concurrent)

# Scheme B source-level V2 baseline. This target compiles the original
# V2 UI classes (MainWindow, *View, *Dialog and custom widgets) instead of
# recreating them from the simplified V3 Designer form.  Its data is seeded by
# v2_source_preview_main.cpp so the visual baseline is deterministic.
set(V2_PROGRAM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../05_workspace_v2/04_program")
if(EXISTS "${V2_PROGRAM_ROOT}/ui/MainWindow.cpp")
    file(GLOB_RECURSE V2_PREVIEW_SOURCES CONFIGURE_DEPENDS
        "${V2_PROGRAM_ROOT}/src/*.cpp"
        "${V2_PROGRAM_ROOT}/ui/*.cpp"
        "${V2_PROGRAM_ROOT}/ui/widgets/*.cpp"
        "${V2_PROGRAM_ROOT}/database/*.cpp"
        "${V2_PROGRAM_ROOT}/service/*.cpp"
        "${V2_PROGRAM_ROOT}/models/*.cpp"
        "${V2_PROGRAM_ROOT}/utils/*.cpp")
    list(REMOVE_ITEM V2_PREVIEW_SOURCES "${V2_PROGRAM_ROOT}/src/main.cpp")

    add_executable(NiumaV2SourcePreview WIN32
        v2_source_preview_main.cpp
        SchemeBStyler.cpp
        SchemeBStyler.h
        ${V2_PREVIEW_SOURCES}
        "${V2_PROGRAM_ROOT}/resources/resources.qrc"
        resources.qrc)
    target_link_libraries(NiumaV2SourcePreview PRIVATE Qt5::Widgets Qt5::Sql Qt5::Concurrent Qt5::Test)
    target_compile_definitions(NiumaV2SourcePreview PRIVATE
        NIUMA_PREVIEW_DATA_DIR=\"${CMAKE_SOURCE_DIR}/06_test/data/ui-preview-debug/v2-preview-data-b\")
    target_include_directories(NiumaV2SourcePreview PRIVATE
        "${V2_PROGRAM_ROOT}"
        "${V2_PROGRAM_ROOT}/src"
        "${V2_PROGRAM_ROOT}/ui"
        "${V2_PROGRAM_ROOT}/ui/widgets"
        "${V2_PROGRAM_ROOT}/database"
        "${V2_PROGRAM_ROOT}/service"
        "${V2_PROGRAM_ROOT}/models"
        "${V2_PROGRAM_ROOT}/utils")
    if(MINGW)
        target_link_libraries(NiumaV2SourcePreview PRIVATE dwmapi)
        target_link_options(NiumaV2SourcePreview PRIVATE -mwindows)
        target_compile_options(NiumaV2SourcePreview PRIVATE -Wall -Wextra)
    elseif(MSVC)
        target_compile_options(NiumaV2SourcePreview PRIVATE /utf-8)
    endif()
endif()
