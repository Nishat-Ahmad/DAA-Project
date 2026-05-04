cmake_minimum_required(VERSION 3.16)

project(ccn2_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ccn2_cpp
    src/main.cpp
    src/dataset.cpp
    src/miners.cpp
    src/benchmark.cpp
)

target_include_directories(ccn2_cpp PRIVATE include)

if (WIN32)
    target_link_libraries(ccn2_cpp PRIVATE Psapi)
endif()
