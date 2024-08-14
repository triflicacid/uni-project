#pragma once
#include <vector>

#include "instruction.hpp"

namespace assembler::instruction::transform {
  void branch(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void exit(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void jump(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void loadw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void zero(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);
}
