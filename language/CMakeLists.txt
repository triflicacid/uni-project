cmake_minimum_required(VERSION 3.6)
project(language LANGUAGES CXX)

include_directories("src")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(language ../shared/util.cpp ../shared/messages/message.cpp ../shared/messages/MessageWithSource.cpp ../shared/messages/list.cpp ../shared/istream_wrapper.cpp ../shared/uint64.cpp
        src/config.cpp
        src/lexer.cpp src/parser.cpp
        src/ast/node.cpp
        src/ast/expr/operator.cpp src/ast/expr/literal.cpp src/ast/expr/symbol_reference.cpp
        src/ast/types/int.cpp src/ast/types/namespace.cpp src/ast/types/float.cpp src/ast/types/function.cpp
        src/symbol/namespace.cpp
        main.cpp)
