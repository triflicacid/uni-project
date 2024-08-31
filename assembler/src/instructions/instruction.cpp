#include <iostream>
#include <utility>

#include "instruction.hpp"

#include <processor/src/arg.h>
#include <processor/src/constants.h>

namespace assembler::instruction {
  void Instruction::print() const {
    std::cout << "Signature '" << signature->mnemonic;

    for (ArgumentType type : signature->arguments[overload]) {
      std::cout << ' ' << Argument::type_to_string(type);
    }

    std::cout << "'; Opcode = 0x" << std::hex << (int) signature->opcode << std::dec << "; "
        << args.size() << " argument(s)\n";

    for (Argument arg : args) {
      std::cout << "\t- ";
      arg.print();
      std::cout << '\n';
    }
  }

  Instruction::Instruction(const Signature *signature, std::deque<Argument> arguments) {
    this->signature = signature;
    args = std::move(arguments);
    overload = 0;
    test = 0x0;
    datatype = 0x0;
  }

  void Instruction::set_conditional_test(uint8_t mask) {
    test = 0x80 | mask; // indicate test is provided
  }

  void Instruction::set_datatype_specifier(uint8_t mask) {
    datatype = mask;
  }

  void Instruction::offset_addresses(uint16_t offset) {
    for (auto &arg : args) {
      if (arg.get_type() == ArgumentType::Address) {
        arg.set_data(arg.get_data() + offset);
      }
    }
  }

  uint64_t Instruction::compile() const {
    InstructionBuilder builder;

    // add opcode
    builder.opcode(signature->opcode);

    // add conditional test?
    if (signature->expect_test) {
      if (test & 0x80) {
        builder.conditional_test(test & 0x3f);
      } else {
        builder.no_conditional_test();
      }
    }

    // add datatype specifier?
    if (signature->expect_datatype) {
      builder.data_type(datatype & 0x7f);
    }

    // add arguments, formatted depending on type
    for (uint8_t i = 0; i < args.size(); i++) {
      const auto &arg = args[i];

      // prepare builder for argument type
      switch (signature->arguments[overload][i]) {
        case ArgumentType::Address:
          builder.next_as_addr();
          break;
        case ArgumentType::Value:
          builder.next_as_value();
          break;
        default: ;
      }

      switch (arg.get_type()) {
        case ArgumentType::Address:
          builder.arg_addr(arg.get_data());
          break;
        case ArgumentType::Immediate:
          builder.arg_imm(arg.get_data());
          break;
        case ArgumentType::DecimalImmediate:
          if (signature->is_full_word) {
            builder.arg_imm(arg.get_data());
          } else {
            // data is a double, so convert to float before insertion
            uint64_t data = arg.get_data();
            auto decimal = (float) *(double *) &data;
            builder.arg_imm(*(uint32_t *) &decimal);
          }
          break;
        case ArgumentType::Register:
          builder.arg_reg(arg.get_data());
          break;
        case ArgumentType::RegisterIndirect:
          builder.arg_reg_indirect(arg.get_reg_indirect()->reg, arg.get_reg_indirect()->offset);
          break;
        default: ;
      }
    }

    return builder.get();
  }

  void InstructionBuilder::opcode(uint8_t opcode) {
    write(OPCODE_SIZE, opcode & OPCODE_MASK);
  }

  void InstructionBuilder::write(uint8_t length, uint64_t data) {
    m_word |= data << m_pos;
    m_pos += length;
  }

  void InstructionBuilder::next_as_value() {
    m_next = NextArgument::AsValue;
  }

  void InstructionBuilder::next_as_addr() {
    m_next = NextArgument::AsAddress;
  }

  void InstructionBuilder::no_conditional_test() {
    write(4, CMP_NA);
  }

  void InstructionBuilder::conditional_test(uint8_t bits) {
    write(4, bits & 0xf);
  }

  void InstructionBuilder::data_type(uint8_t bits) {
    write(3, bits & 0x7);
  }

  void InstructionBuilder::arg_reg(uint8_t reg) {
    switch (m_next) {
      case NextArgument::None:
        write(8, reg);
        return;
      case NextArgument::AsValue:
        write(2, ARG_REG);
        write(32, reg);
        break;
      case NextArgument::AsAddress:
        write(1, ARG_REG_INDIRECT & 0x1);
        write(32, reg);
        break;
    }

    m_next = NextArgument::None;
  }

  void InstructionBuilder::arg_imm(uint32_t imm) {
    write(2, ARG_IMM);
    write(32, imm);

    m_next = NextArgument::None;
  }

  void InstructionBuilder::arg_addr(uint32_t addr) {
    switch (m_next) {
      case NextArgument::None:
        return;
      case NextArgument::AsValue:
        write(2, ARG_MEM);
        break;
      case NextArgument::AsAddress:
        write(1, ARG_MEM & 0x1);
        break;
    }

    write(32, addr);

    m_next = NextArgument::None;
  }

  void InstructionBuilder::arg_reg_indirect(uint8_t reg, int32_t offset) {
    switch (m_next) {
      case NextArgument::None:
        return;
      case NextArgument::AsValue:
        write(2, ARG_REG_INDIRECT);
        break;
      case NextArgument::AsAddress:
        write(1, ARG_REG_INDIRECT & 0x1);
        break;
    }

    write(8, reg);
    write(24, *(uint32_t *) &offset & 0xffffff);

    m_next = NextArgument::None;
  }
}
