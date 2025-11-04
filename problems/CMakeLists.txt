# Find GoogleTest
find_package(GTest REQUIRED)

function(add_problem problem_name)
    file(GLOB_RECURSE PROBLEM_SOURCES CONFIGURE_DEPENDS ${problem_name}/*.cpp)
    add_executable(${problem_name} ${PROBLEM_SOURCES})
    target_link_libraries(${problem_name} GTest::gtest)
    message(STATUS "Added problem: ${problem_name}")
endfunction()

file(GLOB PROBLEM_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
foreach(problem_dir ${PROBLEM_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${problem_dir})
        add_problem(${problem_dir})
    endif()
endforeach()
