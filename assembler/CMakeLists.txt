cmake_minimum_required(VERSION 3.6)
project(assembler LANGUAGES CXX)

include_directories(src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(assembler
        ../shared/util.cpp src/assembler_data.cpp src/chunk.cpp src/parser.cpp
        src/instructions/argument.cpp src/instructions/instruction.cpp src/instructions/variables.cpp
        src/instructions/extra.cpp ../shared/messages/message.cpp ../shared/messages/list.cpp
        ../shared/constants.cpp src/pre-process/data.cpp src/pre-process/pre-processor.cpp main.cpp)
