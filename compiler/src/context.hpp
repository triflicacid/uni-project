#pragma once

#include "messages/list.hpp"
#include "assembly/program.hpp"
#include "memory/stack.hpp"
#include "memory/reg_alloc.hpp"
#include "control_flow/loop_context.hpp"

/** @brief The Edel compiler: lexing, parsing, type-checking, and assembly code generation. */
namespace lang {
  /**
   * @brief Aggregate of the compiler's shared mutable state, threaded by reference through every phase of the compilation pipeline (process, resolve, generate_code) after parsing.
   */
  struct Context {
    message::List& messages; ///< Diagnostic messages accumulated during compilation.
    assembly::Program& program; ///< Output program instructions are emitted into.
    memory::StackManager& stack_manager; ///< Tracks the runtime stack layout during code generation.
    memory::RegisterAllocationManager& reg_alloc_manager; ///< Manages register allocation and spilling during code generation.
    symbol::SymbolTable& symbols; ///< Declared symbols and their storage locations.
    std::stack<control_flow::LoopContext> loops; ///< Stack of enclosing loop contexts, innermost on top.
  };
}
