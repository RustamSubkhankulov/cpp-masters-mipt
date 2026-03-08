cmake_minimum_required(VERSION 3.21)

project(
        cpp-masters
        DESCRIPTION "Solving problems for MIPT C++ programming course"
        LANGUAGES CXX
        )

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CLANG_CXX_PATH clang++)
if (CLANG_CXX_PATH)
    set(CMAKE_CXX_COMPILER "clang++")
    message(STATUS "clang++ found: ${CMAKE_CXX_COMPILER}")
else()
    message(STATUS "clang++ not found")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
  message(STATUS "CMAKE_BUILD_TYPE is not specified, using Debug by default")
endif()

set(WARNING_CXX_FLAGS " -Wall -Wextra -Wpedantic")

string(APPEND CMAKE_CXX_FLAGS ${WARNING_CXX_FLAGS})

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 -g -fsanitize=address -fsanitize=undefined")
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O2")

add_subdirectory(problems)

find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        *.cpp
        *.hpp
    )

    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT} -style=file -i ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
