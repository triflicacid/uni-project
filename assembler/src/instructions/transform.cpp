#include "transform.hpp"

#include <processor/src/constants.h>

namespace assembler::instruction::transform {
  void transform_reg_val(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    if (instruction->args.size() == 1) {
      // copy <reg> as <value>
      instruction->args.emplace_back(instruction->args[0]);
      instruction->overload++;
    }

    instructions.push_back(instruction);
  }

  void transform_reg_val_val(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    if (instruction->args.size() == 2) {
      // copy <reg> as <value>
      instruction->args.emplace_front(instruction->args[0]);
      instruction->overload++;
    }

    instructions.push_back(instruction);
  }

  void branch(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "b $addr"
    // "load $ip, $addr"
    instruction->signature = &Signature::_load;
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
      code_instruction->signature = &Signature::_load;
      code_instruction->args.emplace_back(ArgumentType::Register, REG_RET);
      code_instruction->args.emplace_back(ArgumentType::Immediate, code);
      instructions.push_back(code_instruction);
    }

    // "syscall <opcode: exit>"
    instruction->signature = &Signature::_syscall;
    instruction->args.emplace_back(ArgumentType::Immediate, SYSCALL_EXIT);
    instructions.push_back(instruction);
  }

  void interrupt(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "int $v"
    // "load $k1, $v"
    instruction->signature = &Signature::_load;
    instruction->args.emplace_front(ArgumentType::Register, REG_K1);
    instructions.push_back(instruction);

    // "loadu $k1, $v[32:]"
    instruction = new Instruction(*instruction);
    instruction->signature = &Signature::_loadu;
    instruction->args[1].set_data(instruction->args[1].get_data() >> 32);
    instructions.push_back(instruction);

    // "or $isr, $k1"
    instruction = new Instruction(*instruction);
    instruction->signature = &Signature::_or;
    instruction->args[0].set_data(REG_ISR);
    instruction->args[1].update(ArgumentType::Register, REG_ISR);
    instruction->args.emplace_back(ArgumentType::Register, REG_K1);
    instructions.push_back(instruction);
  }

  void interrupt_return(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "rti"
    // "load $ip, $iip"
    instruction->signature = &Signature::_load;
    instruction->args.emplace_back(ArgumentType::Register, REG_IP);
    instruction->args.emplace_back(ArgumentType::Register, REG_IIP);
    instructions.push_back(instruction);

    // "xor $flag, <in interrupt>"
    instruction = new Instruction(*instruction);
    instruction->signature = &Signature::_xor;
    instruction->args[0].update(ArgumentType::Register, REG_FLAG);
    instruction->args[1].update(ArgumentType::Register, REG_FLAG);
    instruction->args.emplace_back(ArgumentType::Immediate, FLAG_IN_INTERRUPT);
    instructions.push_back(instruction);
  }

  void jump(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    branch(instructions, instruction, overload);
  }

  void loadw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "loadl $r, $v"
    // "load $r, $v"
    instruction->signature = &Signature::_load;
    instructions.push_back(instruction);

    // "loadu $r, $v[32:]"
    instruction = new Instruction(*instruction);
    instruction->signature = &Signature::_loadu;
    instruction->args[1].set_data(instruction->args[1].get_data() >> 32);
    instructions.push_back(instruction);
  }

  void pushw(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "pushw $v"
    // "push $r, $v"
    instruction->signature = &Signature::_push;
    instructions.push_back(instruction);

    // "push $v[32:]"
    instruction = new Instruction(*instruction);
    instruction->args[0].set_data(instruction->args[0].get_data() >> 32);
    instructions.push_back(instruction);
  }

  void zero(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    // original: "zero $r"
    // "load $r, 0"
    instruction->signature = &Signature::_load;
    instruction->args.emplace_back(ArgumentType::Immediate, 0);
    instructions.push_back(instruction);
  }
}
