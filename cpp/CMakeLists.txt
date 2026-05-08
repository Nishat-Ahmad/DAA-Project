cmake_minimum_required(VERSION 3.16)

project(ccbench_cpp LANGUAGES CXX)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

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

if (ipo_supported)
    set_property(TARGET ccbench_cpp PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
    message(WARNING "IPO/LTO is not supported: ${ipo_error}")
endif()
