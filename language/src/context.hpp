#pragma once

#include "messages/list.hpp"
#include "assembly/program.hpp"
#include "memory/stack.hpp"
#include "memory/reg_alloc.hpp"

namespace lang {
  struct Context {
    message::List& messages;
    assembly::Program& program;
    memory::StackManager& stack_manager;
    memory::RegisterAllocationManager& reg_alloc_manager;
    symbol::SymbolTable& symbols;
  };
}
