cmake_minimum_required(VERSION 3.6)
project(language LANGUAGES CXX)

include_directories("src")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(language ../shared/constants.cpp ../shared/util.cpp ../shared/messages/message.cpp ../shared/messages/MessageWithSource.cpp ../shared/messages/list.cpp ../shared/istream_wrapper.cpp ../shared/uint64.cpp
        src/config.cpp src/message_helper.cpp src/parser.cpp
        src/lexer/lexer.cpp
        src/memory/literal.cpp src/memory/reg_alloc.cpp src/memory/stack.cpp
        src/ast/block.cpp src/ast/expr.cpp src/ast/function_base.cpp src/ast/function.cpp src/ast/namespace.cpp src/ast/node.cpp src/ast/program.cpp src/ast/return.cpp src/ast/symbol_declaration.cpp
        src/assembly/arg.cpp src/assembly/basic_block.cpp src/assembly/create.cpp src/assembly/directive.cpp src/assembly/instruction.cpp src/assembly/program.cpp
        src/ast/expr/operator.cpp src/ast/expr/literal.cpp src/ast/expr/symbol_reference.cpp
        src/ast/types/node.cpp src/ast/types/none.cpp src/ast/types/graph.cpp src/ast/types/int.cpp src/ast/types/namespace.cpp src/ast/types/float.cpp src/ast/types/function.cpp
        src/symbol/registry.cpp src/symbol/symbol.cpp src/symbol/table.cpp
        src/operators/builtins.cpp src/operators/operator.cpp
        main.cpp)
