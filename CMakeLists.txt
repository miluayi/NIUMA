cmake_minimum_required(VERSION 3.16)

project(NIUMA_V3 VERSION 3.1.5 LANGUAGES CXX)

include(CTest)

option(NIUMA_BUILD_UI_PREVIEW "Build the NIUMA source preview and contract tests" ON)
option(NIUMA_BUILD_PRODUCTION "Build the isolated v3 production migration skeleton" OFF)

if(NIUMA_BUILD_UI_PREVIEW)
    add_subdirectory(04_program/ui_preview)
endif()

if(NIUMA_BUILD_PRODUCTION)
    add_subdirectory(04_program)
endif()
