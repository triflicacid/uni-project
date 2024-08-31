#pragma once
#include <vector>
#include <messages/list.hpp>

#include "instruction.hpp"
#include "extra.hpp"
#include "assembler_data.hpp"

namespace assembler::instruction::transform {
  // generic transform -- transform reg to reg_reg
  // e.g., for use with { reg, reg_reg }
  void transform_reg_reg(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  // generic transform -- transform reg_val to reg_reg_val
  // e.g., for use with { reg_val, reg_reg_val }
  void transform_reg_reg_val(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void branch(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void exit(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void interrupt(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void interrupt_return(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void jump(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void loadw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void pushw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);

  void zero(std::vector<Instruction *> &instructions, Instruction *instruction, int overload);
}

namespace assembler::instruction::parse {
  // cvt(d1)2(d2)
  void convert(const Data &data, int line_idx, int &col, Instruction *instruction, std::string &options, message::List &msgs);
}
