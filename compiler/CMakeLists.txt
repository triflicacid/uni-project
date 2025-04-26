cmake_minimum_required(VERSION 3.6)
project(compiler LANGUAGES CXX)

include_directories("src")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../out)
add_executable(compiler ../shared/constants.cpp ../shared/util.cpp ../shared/messages/message.cpp ../shared/messages/MessageWithSource.cpp ../shared/messages/list.cpp ../shared/istream_wrapper.cpp ../shared/uint64.cpp
        src/config.cpp src/message_helper.cpp src/parser.cpp
        src/lexer/lexer.cpp
        src/memory/literal.cpp src/memory/reg_alloc.cpp src/memory/stack.cpp src/memory/storage_location.cpp
        src/assembly/arg.cpp src/assembly/basic_block.cpp src/assembly/create.cpp src/assembly/directive.cpp src/assembly/instruction.cpp src/assembly/program.cpp
        src/ast/block.cpp src/ast/namespace.cpp src/ast/node.cpp src/ast/program.cpp src/ast/symbol_declaration.cpp
        src/ast/control-flow/loop_control.cpp src/ast/control-flow/loop_statement.cpp src/ast/control-flow/if_statement.cpp src/ast/control-flow/while_statement.cpp
        src/ast/function/function_base.cpp src/ast/function/function.cpp src/ast/function/operator_definition.cpp src/ast/function/return.cpp
        src/types/array.cpp src/types/bool.cpp src/types/node.cpp src/types/pointer.cpp src/types/unit.cpp src/types/graph.cpp src/types/int.cpp src/types/namespace.cpp src/types/float.cpp src/types/function.cpp src/types/wrapper.cpp
        src/ast/leaves/array_literal.cpp src/ast/operator.cpp src/ast/leaves/literal.cpp src/ast/leaves/symbol_reference.cpp src/ast/leaves/unit.cpp
        src/control-flow/conditional_context.cpp
        src/symbol/function.cpp src/symbol/registry.cpp src/symbol/symbol.cpp src/symbol/table.cpp
        src/operators/builtins.cpp src/operators/operator.cpp
        src/value/value.cpp
        main.cpp)
