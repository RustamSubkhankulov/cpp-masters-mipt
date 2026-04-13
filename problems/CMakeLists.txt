find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

function(add_problem problem_name)
    file(GLOB_RECURSE PROBLEM_SOURCES CONFIGURE_DEPENDS ${problem_name}/*.cpp)
    add_executable(${problem_name} ${PROBLEM_SOURCES})
    target_link_libraries(${problem_name} GTest::gtest)
    message(STATUS "Added problem: ${problem_name}")
endfunction()

file(GLOB PROBLEM_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
foreach(problem_dir ${PROBLEM_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${problem_dir}
       AND NOT problem_dir STREQUAL "06-04"
       AND NOT problem_dir STREQUAL "06-05")
        add_problem(${problem_dir})
    endif()
endforeach()

target_link_libraries(07-05 benchmark::benchmark)
target_link_libraries(09-09 benchmark::benchmark)

target_compile_options(10-05 PRIVATE -O3 -m32)
target_link_options(10-05 PRIVATE -m32)

add_subdirectory(06-05)