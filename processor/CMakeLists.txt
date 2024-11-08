cmake_minimum_required(VERSION 3.6)
project(processor LANGUAGES C)

include_directories(src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(processor src/arg.c src/bus.c src/cpu.c src/dram.c main.c)

project(processor-test LANGUAGES C)

add_executable(processor-test src/bus.c src/cpu.c src/dram.c test.c)
