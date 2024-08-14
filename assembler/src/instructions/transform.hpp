#pragma once
#include <vector>

#include "instruction.hpp"

namespace assembler::instruction::transform {
  void exit(std::vector<Instruction *> &instructions, Instruction *instruction);

  void jump(std::vector<Instruction *> &instructions, Instruction *instruction);

  void loadw(std::vector<Instruction *> &instructions, Instruction *instruction);

  void zero(std::vector<Instruction *> &instructions, Instruction *instruction);
}
