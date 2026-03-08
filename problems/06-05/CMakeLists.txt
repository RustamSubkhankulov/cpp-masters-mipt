find_package(Boost REQUIRED COMPONENTS filesystem)

function(add_problem_06_05_library target_name source_file)
    add_library(${target_name} SHARED ${source_file})

    target_include_directories(${target_name} PRIVATE ${Boost_INCLUDE_DIRS})
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)

    set_target_properties(
        ${target_name}
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endfunction()

add_problem_06_05_library(
    problem_06_05_library_v1
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/version1.cpp
)

add_problem_06_05_library(
    problem_06_05_library_v2
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/version2.cpp
)

add_executable(06-05 ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_include_directories(06-05 PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_options(06-05 PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(
    06-05
    PRIVATE
    GTest::gtest
    Boost::filesystem
    ${CMAKE_DL_LIBS}
)

add_dependencies(06-05 problem_06_05_library_v1 problem_06_05_library_v2)
