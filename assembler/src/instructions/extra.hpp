#pragma once

#include <vector>
#include <messages/list.hpp>

#include "instruction.hpp"
#include "extra.hpp"
#include "assembler_data.hpp"

namespace assembler::instruction::transform {
    // generic transform -- transform reg to reg_reg
    // e.g., for use with { reg, reg_reg }
    void
    transform_reg_reg(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                      int overload);

    // generic transform -- transform reg_val to reg_reg_val
    // e.g., for use with { reg_val, reg_reg_val }
    void transform_reg_reg_val(std::vector<std::unique_ptr<Instruction>> &instructions,
                               std::unique_ptr<Instruction> instruction, int overload);

    void
    transform_jal(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                  int overload);

    void branch(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                int overload);

    void exit(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
              int overload);

    void interrupt(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                   int overload);

    void
    interrupt_return(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                     int overload);

    void jump(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
              int overload);

    void
    load_immediate(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                   int overload);

    void zero(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
              int overload);
}

namespace assembler::instruction::parse {
    // cvt(d1)2(d2)
    void
    convert(const Data &data, Location &loc, std::unique_ptr<Instruction> &instruction, std::string &options,
            message::List &msgs);
}
