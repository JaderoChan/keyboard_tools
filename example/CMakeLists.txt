cmake_minimum_required(VERSION 3.25)

project(keyboard_tools_example LANGUAGES CXX)
set(KBDT_MSG_PREFIX "[Keyboard Tools Example]")
message(STATUS "${KBDT_MSG_PREFIX} Example will be built.")

if(NOT TARGET keyboard_tools)
    find_package(keyboard_tools REQUIRED)
endif()

if(NOT KEYBOARD_TOOLS_WITH_KEYUTIL)
    message(FATAL_ERROR "${KBDT_MSG_PREFIX} Can't build example with no key utility.")
endif()

link_libraries(keyboard_tools::keyboard_tools)

option(KEYBOARD_TOOLS_EXAMPLE_BUILD_ECHO "Build echo example" ON)
option(KEYBOARD_TOOLS_EXAMPLE_BUILD_QUICK_INPUT_PI "Build quick input pi example" ON)

if(KEYBOARD_TOOLS_EXAMPLE_BUILD_ECHO)
    add_subdirectory(echo)
endif()
if(KEYBOARD_TOOLS_EXAMPLE_BUILD_QUICK_INPUT_PI)
    add_subdirectory(quick_input_pi)
endif()

if(NOT LINUX)
    option(KEYBOARD_TOOLS_EXAMPLE_BUILD_DISABLE_KEY "Build disable key example" ON)
    option(KEYBOARD_TOOLS_EXAMPLE_BUILD_REMAP_KEY "Build remap key example" ON)

    if(KEYBOARD_TOOLS_EXAMPLE_BUILD_DISABLE_KEY)
        add_subdirectory(disable_key)
    endif()
    if(KEYBOARD_TOOLS_EXAMPLE_BUILD_REMAP_KEY)
        add_subdirectory(remap_key)
    endif()
endif()
