cmake_minimum_required(VERSION 3.16)
project(cache_aware_matmul LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)
target_compile_options(project_options INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-O2;-Wall;-Wextra;-Wpedantic>)
target_include_directories(project_options INTERFACE include)

add_executable(matmul_demo src/main.cpp)
target_link_libraries(matmul_demo PRIVATE project_options)

add_executable(matmul_benchmark src/benchmark.cpp)
target_link_libraries(matmul_benchmark PRIVATE project_options)

add_executable(matmul_tests tests/test_matmul.cpp)
target_link_libraries(matmul_tests PRIVATE project_options)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME matmul_correctness COMMAND matmul_tests)
endif()
