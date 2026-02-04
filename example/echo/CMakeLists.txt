cmake_minimum_required(VERSION 3.25)

project(echo LANGUAGES CXX)

if(NOT TARGET keyboard_tools)
    find_package(keyboard_tools REQUIRED)
endif()

if(NOT KEYBOARD_TOOLS_WITH_KEYUTILS)
    message(FATAL_ERROR "${KBDT_MSG_PREFIX} Can't build example with no key utilitys.")
endif()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE keyboard_tools::keyboard_tools)
