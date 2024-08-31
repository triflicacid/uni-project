#pragma once
#include <vector>

#include "instruction.hpp"

namespace assembler::instruction::transform {
  // generic transform -- transform reg to reg_val
  // e.g., for use with { reg, reg_val }
  void transform_reg_val(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  // generic transform -- transform reg_val to reg_val_val
  // e.g., for use with { reg_val, reg_val_val }
  void transform_reg_val_val(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void branch(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void exit(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void interrupt(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void interrupt_return(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void jump(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void loadw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void pushw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void zero(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);
}
