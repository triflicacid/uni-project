cmake_minimum_required(VERSION 3.6)
project(processor LANGUAGES CXX)

include_directories(src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(processor src/bus.cpp src/core.cpp src/cpu.cpp src/debug.cpp src/dram.cpp ../shared/constants.cpp main.cpp)
