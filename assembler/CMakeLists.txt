cmake_minimum_required(VERSION 3.6)
project(assembler LANGUAGES CXX)

include_directories(src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(assembler
        ../util/util.cpp src/assembler_data.cpp src/chunk.cpp src/parser.cpp
        src/instructions/argument.cpp src/instructions/instruction.cpp src/instructions/variables.cpp
        src/instructions/extra.cpp src/messages/message.cpp src/messages/error.cpp src/messages/list.cpp
        src/pre-process/data.cpp src/pre-process/pre-processor.cpp assembler.cpp)


project(assembler-test LANGUAGES CXX)

include_directories(src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(assembler-test ../util/util.cpp test.cpp)
