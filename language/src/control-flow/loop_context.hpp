#pragma once

#include <string>

namespace lang::control_flow {
  struct LoopContext {
    std::string start; // block at the start of the loop
    std::string end; // block at the end of the loop
  };
}
