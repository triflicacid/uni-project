#pragma once
#include <vector>

#include "instruction.hpp"

namespace assembler::instruction::transform {
  // generic transform -- duplicate first register if second arg is not a register
  void duplicate_reg(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void branch(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void exit(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void jump(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void loadw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void zero(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);
}
