#include "transform.hpp"

#include <processor/src/constants.h>

namespace assembler::instruction::transform {
  void duplicate_reg(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    if (!instruction->args.empty() && instruction->args[1].get_type() != ArgumentType::Register) {
      instruction->args.emplace_front(ArgumentType::Register, instruction->args[0].get_data());
      instruction->overload++;
    }

    instructions.push_back(instruction);
  }

  void branch(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "b $addr"
    // "load $ip, $addr"
    instruction->opcode = OP_LOAD;
    instruction->args.emplace_front(ArgumentType::Register, REG_IP);
    instructions.push_back(instruction);
  }

  void exit(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "exit [code]"
    // extract code?
    uint64_t code = 0;
    if (overload) {
      code = instruction->args[0].get_data();
      instruction->args.pop_back();
    }

    // if provided, load code into $ret
    if (overload) {
      auto code_instruction = new Instruction(*instruction);
      code_instruction->opcode = OP_LOAD;
      code_instruction->args.emplace_back(ArgumentType::Register, REG_RET);
      code_instruction->args.emplace_back(ArgumentType::Immediate, code);
      instructions.push_back(code_instruction);
    }

    // "syscall <opcode: exit>"
    instruction->opcode = OP_SYSCALL;
    instruction->args.emplace_back(ArgumentType::Immediate, SYSCALL_EXIT);
    instructions.push_back(instruction);
  }

  void jump(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    branch(instructions, instruction, overload);
  }

  void loadw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "loadl $r, $v"
    // "load $r, $v"
    instruction->opcode = OP_LOAD;
    instructions.push_back(instruction);

    // "loadu $r, $v[32:]"
    instruction = new Instruction(*instruction);
    instruction->opcode = OP_LOAD_UPPER;
    instruction->args[1].set_data(instruction->args[1].get_data() >> 32);
    instructions.push_back(instruction);
  }

  void pushw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "pushw $v"
    // "push $r, $v"
    instruction->opcode = OP_PUSH;
    instructions.push_back(instruction);

    // "push $v[32:]"
    instruction = new Instruction(*instruction);
    instruction->opcode = OP_PUSH;
    instruction->args[0].set_data(instruction->args[0].get_data() >> 32);
    instructions.push_back(instruction);
  }

  void zero(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "zero $r"
    // "load $r, 0"
    instruction->opcode = OP_LOAD;
    instruction->args.emplace_back(ArgumentType::Immediate, 0);
    instructions.push_back(instruction);
  }
}
