
cmake_minimum_required(VERSION 3.14)
project(tests LANGUAGES CXX)

find_package(GTest 1.17.0 REQUIRED)

message(STATUS "GTest include dirs: ${GTEST_INCLUDE_DIRS}")
message(STATUS "GTest libraries: ${GTEST_LIBRARIES}")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

add_compile_options(
    -g
    -Wall -Wextra -pedantic -Wliteral-conversion
    -Wno-macro-redefined -Wno-unused-variable -Wno-unused-parameter -Wno-char-subscripts -Wno-comment -Wno-sign-compare
    -Wno-gnu-zero-variadic-macro-arguments
    -fno-math-errno -fno-trapping-math -fassociative-math -freciprocal-math -fno-signed-zeros
    -fno-exceptions -fno-rtti
    -DMACOS
    -DDEBUG_LEVEL=2 -DMEMORY_DEBUG_LEVEL=0
)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(tests 
    ${SRC_FILES})

target_link_libraries(tests PRIVATE
    faketorio_lib
    GTest::GTest
    GTest::gtest_main
)
target_include_directories(tests PRIVATE
    ../include
)

include(GoogleTest)
gtest_discover_tests(tests)