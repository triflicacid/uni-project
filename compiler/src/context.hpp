#pragma once

#include "messages/list.hpp"
#include "assembly/program.hpp"
#include "memory/stack.hpp"
#include "memory/reg_alloc.hpp"
#include "control_flow/loop_context.hpp"

namespace lang {
  /**
   * @brief Aggregate of the compiler's shared mutable state, threaded by reference through every phase of the compilation pipeline (process, resolve, generate_code) after parsing.
   */
  struct Context {
    message::List& messages;
    assembly::Program& program;
    memory::StackManager& stack_manager;
    memory::RegisterAllocationManager& reg_alloc_manager;
    symbol::SymbolTable& symbols;
    std::stack<control_flow::LoopContext> loops; // track which loops we are in
  };
}
