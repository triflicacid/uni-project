#include "extra.hpp"
#include "signature.hpp"

#include <util.hpp>
#include <processor/src/constants.h>

namespace assembler::instruction::transform {
  void transform_reg_reg(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    if (instruction->args.size() == 1) {
      // duplicate <reg>
      instruction->args.emplace_back(instruction->args[0]);
      instruction->overload++;
    }

    instructions.push_back(instruction);
  }

  void transform_reg_reg_val(std::vector<Instruction *> &instructions, Instruction *instruction, int overload) {
    if (instruction->args.size() == 2) {
      // duplicate <reg>
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
    uint64_t imm = instruction->args[1].get_data();

    // "load $r, $v[:32]"
    instruction->signature = &Signature::_load;
    instruction->args[1].update(ArgumentType::Immediate, imm & 0xffffffff);
    instructions.push_back(instruction);

    // "loadu $r, $v[32:]"
    instruction = new Instruction(*instruction);
    instruction->signature = &Signature::_loadu;
    instruction->args[1].set_data(imm >> 32);
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

namespace assembler::instruction::parse {
  void convert(const Data &data, int line_idx, int &col, Instruction *instruction, std::string &options, message::List &msgs) {
    for (uint8_t i = 0; i < 2; i++) {
      // parse datatype
      bool found = false;

      for (auto &pair : datatype_postfix_map) {
        if (starts_with(options, pair.first)) {
          found = true;
          instruction->add_datatype_specifier(pair.second);

          // increase position
          options = options.substr(pair.first.length());

          // if first datatype, expect '2'
          if (i == 0) {
            if (options[0] == '2') {
              options = options.substr(1);
            } else {
              std::string ch(1, options[0]);
              auto err = new message::Message(message::Error, data.file_path, line_idx, col);
              err->set_message("cvt: expected '2' after first datatype, got '" + ch + "'");
              msgs.add(err);

              return;
            }
          }

          break;
        }
      }

      if (!found) {
        auto err = new message::Message(message::Error, data.file_path, line_idx, col);
        err->set_message("cvt: expected datatype. Syntax: cvt(d1)2(d2)");
        msgs.add(err);

        return;
      }
    }
  }
}
