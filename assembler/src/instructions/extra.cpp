#include "extra.hpp"
#include "signature.hpp"

#include <util.hpp>

namespace assembler::instruction::transform {
  void
  transform_reg_reg(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                    int overload) {
    if (instruction->args.size() == 1) {
      // duplicate <reg>
      instruction->args.emplace_back(instruction->args[0]);
      instruction->overload++;
    }

    instructions.push_back(std::move(instruction));
  }

  void transform_reg_reg_val(std::vector<std::unique_ptr<Instruction>> &instructions,
                             std::unique_ptr<Instruction> instruction, int overload) {
    if (instruction->args.size() == 2) {
      // duplicate <reg>
      instruction->args.emplace_front(instruction->args[0]);
      instruction->overload++;
    }

    instructions.push_back(std::move(instruction));
  }

  void transform_last_imm_to_byte(std::vector<std::unique_ptr<Instruction>> &instructions,
                                  std::unique_ptr<Instruction> instruction, int overload) {
    auto &arg = instruction->args[instruction->args.size() - 1];
    arg.update(ArgumentType::Byte, arg.get_data());
    instructions.push_back(std::move(instruction));
  }


  void
  transform_jal(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                int overload) {
    if (instruction->args.size() == 1) {
      // add $rip as register
      instruction->args.emplace_front(ArgumentType::Register, constants::registers::rpc);
      instruction->overload++;
    }

    instructions.push_back(std::move(instruction));
  }

  void branch(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
              int overload) {
    // original: "b $addr"
    // "load $ip, $addr"
    instruction->signature = &Signature::_load;
    instruction->overload = 0;
    instruction->args.emplace_front(ArgumentType::Register, constants::registers::pc);
    instructions.push_back(std::move(instruction));
  }

  void exit(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
            int overload) {
    // original: "exit [code]"
    // extract code?
    uint64_t code = 0;
    if (overload) {
      code = instruction->args[0].get_data();
      instruction->args.pop_back();
    }

    // if provided, load code into $ret
    if (overload) {
      auto code_instruction = std::make_unique<Instruction>(*instruction);
      code_instruction->signature = &Signature::_load;
      code_instruction->overload = 0;
      code_instruction->args.emplace_front(ArgumentType::Immediate, code);
      code_instruction->args.emplace_front(ArgumentType::Register, constants::registers::ret);
      instructions.push_back(std::move(code_instruction));
    }

    // "syscall <opcode: exit>"
    instruction->signature = &Signature::_syscall;
    instruction->overload = 0;
    instruction->args.emplace_back(ArgumentType::Immediate, (int) constants::syscall::exit);
    instructions.push_back(std::move(instruction));
  }

  void interrupt(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                 int overload) {
    // original: "int <value>"
    // "or $isr, <value>"
    instruction->signature = &Signature::_or;
    instruction->overload = 1;
    instruction->args.emplace_front(ArgumentType::Register, constants::registers::isr);
    instruction->args.emplace_front(ArgumentType::Register, constants::registers::isr);
    instructions.push_back(std::move(instruction));
  }

  void
  interrupt_return(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                   int overload) {
    // original: "rti"
    // "load $ip, $iip"
    instruction->signature = &Signature::_load;
    instruction->overload = 0;
    instruction->args.emplace_back(ArgumentType::Register, constants::registers::pc);
    instruction->args.emplace_back(ArgumentType::Register, constants::registers::ipc);
    auto *p = instruction.get();
    instructions.push_back(std::move(instruction));

    // "and $flag, ~<in interrupt>"
    instruction = std::make_unique<Instruction>(*p);
    instruction->signature = &Signature::_and;
    instruction->overload = 1;
    instruction->args[0].update(ArgumentType::Register, constants::registers::flag);
    instruction->args[1].update(ArgumentType::Register, constants::registers::flag);
    instruction->args.emplace_back(ArgumentType::Immediate, ~static_cast<uint32_t>(constants::flag::in_interrupt));
    instructions.push_back(std::move(instruction));
  }

  void jump(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
            int overload) {
    branch(instructions, std::move(instruction), overload);
  }

  void
  load_immediate(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                 int overload) {
    // original: "loadi $r, $i"
    uint64_t imm = instruction->args[1].get_data();

    // "load $r, $i[:32]"
    instruction->signature = &Signature::_load;
    instruction->overload = 0;
    instruction->args[1].update(ArgumentType::Immediate, imm & 0xffffffff);
    auto *p = instruction.get();
    instructions.push_back(std::move(instruction));

    // "loadu $r, $i[32:]"
    instruction = std::make_unique<Instruction>(*p);
    instruction->signature = &Signature::_loadu;
    instruction->overload = 0;
    instruction->args[1].set_data(imm >> 32);
    instructions.push_back(std::move(instruction));
  }

  void zero(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
            int overload) {
    // original: "zero $r"
    // "load $r, 0"
    instruction->signature = &Signature::_load;
    instruction->overload = 0;
    instruction->args.emplace_back(ArgumentType::Immediate, 0);
    instructions.push_back(std::move(instruction));
  }
}

namespace assembler::instruction::parse {
  void
  convert(const Data &data, Location &loc, std::unique_ptr<Instruction> &instruction, std::string &options,
          message::List &msgs) {
    int &col = loc.columnref();

    for (uint8_t i = 0; i < 2; i++) {
      // parse datatype
      bool found = false;

      int j = 0;
      auto dt = constants::inst::datatype::from_string(options, j);

      if (dt) {
        instruction->add_datatype_specifier(dt.value());

        // increase position
        options = options.substr(j);

        // if first datatype, expect '2'
        if (i == 0) {
          if (options[0] == '2') {
            options = options.substr(1);
          } else {
            auto msg = std::make_unique<message::Message>(message::Error, loc);
            msg->get() << "cvt: expected '2' after first datatype, got '" << options[0] << "'";
            msgs.add(std::move(msg));
            return;
          }
        }

        continue;
      }

      auto msg = std::make_unique<message::Message>(message::Error, loc);
      msg->get() << "cvt: expected datatype. Syntax: cvt(d1)2(d2)";
      msgs.add(std::move(msg));
      return;
    }
  }
}
