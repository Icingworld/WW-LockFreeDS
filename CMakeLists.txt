cmake_minimum_required(VERSION 3.5)
project(WW-LockFreeDS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(containers)

option(ENABLE_TEST "Enable testing of the benchmark library." OFF)
option(ENABLE_BENCHMARK "Enable benchmarking of the benchmark library." OFF)

if(ENABLE_TEST)
    message(STATUS "Building tests")
    add_subdirectory(test)
else()
    message(STATUS "Skipping tests")
endif()

if(ENABLE_BENCHMARK)
    message(STATUS "Building benchmark")
    add_subdirectory(benchmark)
else()
    message(STATUS "Skipping benchmark")
endif()

add_subdirectory(third-party)
