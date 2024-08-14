#include "transform.hpp"

#include <processor/src/constants.h>

namespace assembler::instruction::transform {
  void branch(std::vector<Instruction *> &instructions, Instruction *instruction) {
    // original: "b $addr"
    // "load $ip, $addr"
    instruction->opcode = OP_LOAD;
    instruction->args.emplace_front(-1, instruction::ArgumentType::Register, REG_IP);
    instructions.push_back(instruction);
  }

  void exit(std::vector<Instruction *> &instructions, Instruction *instruction) {
    // original: "exit"
    // "syscall <opcode: exit>"
    instruction->opcode = OP_SYSCALL;
    instruction->args.emplace_back(-1, instruction::ArgumentType::Immediate, SYSCALL_EXIT);
    instructions.push_back(instruction);
  }

  void jump(std::vector<Instruction *> &instructions, Instruction *instruction) {
    branch(instructions, instruction);
  }

  void loadw(std::vector<Instruction *> &instructions, Instruction *instruction) {
    // original: "loadl $r, $v"
    // "load $r, $v"
    instruction->opcode = OP_LOAD;
    instructions.emplace_back(instruction);

    // "loadu $r, $v[32:]"
    instruction = new Instruction(*instruction);
    instruction->opcode = OP_LOAD_UPPER;
    instruction->args[1].set_data(instruction->args[1].get_data() >> 32);
    instructions.push_back(instruction);
  }

  void zero(std::vector<Instruction *> &instructions, Instruction *instruction) {
    // original: "zero $r"
    // "load $r, 0"
    instruction->opcode = OP_LOAD;
    instruction->args.emplace_back(-1, instruction::ArgumentType::Immediate, 0);
    instructions.push_back(instruction);
  }
}
