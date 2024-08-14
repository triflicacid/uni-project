#pragma once
#include <vector>

#include "instruction.hpp"

namespace assembler::instruction::transform {
  void zero(std::vector<Instruction *> &instructions, Instruction *instruction);

  void loadw(std::vector<Instruction *> &instructions, Instruction *instruction);
}
