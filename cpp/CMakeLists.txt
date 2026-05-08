cmake_minimum_required(VERSION 3.16)

project(ccbench_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ccbench_cpp
    src/main.cpp
    src/dataset.cpp
    src/apriori.cpp
    src/linear_table.cpp
    src/benchmark.cpp
)

target_include_directories(ccbench_cpp PRIVATE include)

if (WIN32)
    target_link_libraries(ccbench_cpp PRIVATE Psapi)
endif()
