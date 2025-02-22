#pragma once

#include "assembly/basic_block.hpp"

namespace lang::control_flow {
  struct LoopContext {
    assembly::BasicBlock& start; // block at the start of the loop
    assembly::BasicBlock& end; // block at the end of the loop
  };
}
