cmake_minimum_required(VERSION 3.28)
project(processor LANGUAGES C)

include_directories("src")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
add_executable(processor src/bus.c src/dram.c src/cpu.c main.c)
