#include <iostream>
#include <utility>

#include "instruction.hpp"

#include <processor/src/constants.h>

namespace assembler::instruction {
  void Instruction::print() const {
    std::cout << "Mnemonic '" << signature->mnemonic << "'; Opcode = 0x" << std::hex << (int) opcode << std::dec << "; "
        << args.size() << " argument(s)\n";

    for (Argument arg : args) {
      std::cout << "\t- ";
      arg.print();
      std::cout << '\n';
    }
  }

  Instruction::Instruction(const Signature *signature, std::deque<Argument> arguments) {
    this->signature = signature;
    opcode = signature->opcode;
    args = std::move(arguments);
    test = 0x0;
    datatype = 0x0;
  }

  void Instruction::include_test_bits() {
    test = 0x80; // include, no test
  }

  void Instruction::include_test_bits(uint8_t mask) {
    test = 0xc0 | (mask & 0x3f); // include, perform test with given mask
  }

  void Instruction::include_datatype_specifier(uint8_t mask) {
    datatype = 0x80 | (mask & 0x7f); // enable specifier with mask
  }

  uint64_t Instruction::compile() const {
    InstructionBuilder builder;

    // add opcode
    builder.opcode(opcode);

    // add conditional test?
    if (test & 0x80) {
      if (test & 0x40) {
        builder.conditional_test(test & 0x3f);
      } else {
        builder.no_conditional_test();
      }
    }

    // add datatype specifier?
    if (datatype & 0x80) {
      builder.data_type(datatype & 0x7f);
    }

    // add arguments, formatted depending on type
    for (auto &arg : args) {
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
        case ArgumentType::RegisterValue:
          builder.arg_reg(arg.get_data(), arg.get_type() == ArgumentType::RegisterValue);
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

  void InstructionBuilder::no_conditional_test() {
    write(4, CMP_NA);
  }

  void InstructionBuilder::conditional_test(uint8_t bits) {
    write(4, bits & 0xf);
  }

  void InstructionBuilder::data_type(uint8_t bits) {
    write(3, bits & 0x7);
  }

  void InstructionBuilder::arg_reg(uint8_t reg, bool as_value) {
    if (!as_value) {
      write(8, reg);
      return;
    }

    write(2, ARG_REG);
    write(32, reg);
  }

  void InstructionBuilder::arg_imm(uint32_t imm) {
    write(2, ARG_IMM);
    write(32, imm);
  }

  void InstructionBuilder::arg_addr(uint32_t addr) {
    write(2, ARG_MEM);
    write(32, addr);
  }

  void InstructionBuilder::arg_reg_indirect(uint8_t reg, int32_t offset) {
    write(2, ARG_REG_INDIRECT);
    write(8, reg);
    write(24, *(uint32_t *) &offset & 0xffffff);
  }
}
