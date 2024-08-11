#include <iostream>
#include <utility>

#include "instruction.hpp"

#include <util.hpp>

namespace assembler::instruction {
  std::map<std::string, uint8_t> conditional_postfix_map = {
    { "z", CMP_Z },
    { "nz", CMP_NZ },
    { "eq", CMP_EQ },
    { "ne", CMP_NE },
    { "lt", CMP_LT },
    { "le", CMP_LE },
    { "gt", CMP_GT },
    { "ge", CMP_GE },
    };

  std::map<std::string, uint8_t> datatype_postfix_map = {
    { "hu", DATATYPE_U32 },
    { "u", DATATYPE_U64 },
    { "hs", DATATYPE_S32 },
    { "s", DATATYPE_S64 },
    { "f", DATATYPE_F },
    { "d", DATATYPE_D }
  };

  std::map<std::string, uint8_t> opcode_map = {
    { "nop", OP_NOP },
  };

  std::map<std::string, Signature> signature_map = {
    { "load", { OP_LOAD, true, false, { ArgumentType::Register, ArgumentType::Value } } },
  };


  uint8_t *find_opcode(const std::string &mnemonic, std::string &options) {
    for (auto &pair : opcode_map) {
      if (starts_with(pair.first, mnemonic)) {
        options = mnemonic.substr(pair.first.size());
        return &pair.second;
      }
    }

    auto signature = find_signature(mnemonic, options);
    return signature == nullptr ? nullptr : &signature->opcode;
  }

  Signature *find_signature(const std::string &mnemonic, std::string &options) {
    for (auto &pair : signature_map) {
      if (starts_with(pair.first, mnemonic)) {
        options = mnemonic.substr(pair.first.size());
        return &pair.second;
      }
    }

    return nullptr;
  }

  void Instruction::print() {
    std::cout << "Mnemonic \"" << *mnemonic << "\"; Opcode = 0x" << std::hex << (int) opcode << std::dec << "; "
        << args.size() << " argument(s)\n";

    for (Argument arg : args) {
      std::cout << "\t- ";
      arg.print();
      std::cout << '\n';
    }
  }

  Instruction::Instruction(const std::string *mnemonic, uint8_t opcode, std::vector<Argument> arguments) {
    this->mnemonic = mnemonic;
    this->opcode = opcode;
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

  uint64_t Instruction::compile() {
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
        case ArgumentType::ImmediateValue:
          builder.arg_imm(arg.get_data());
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
    // m_word = (m_word << length) | data;
    m_word |= data << m_pos;
    m_pos += length;
  }

  void InstructionBuilder::no_conditional_test() {
    write(4, 0x0);
  }

  void InstructionBuilder::conditional_test(uint8_t bits) {
    write(4, 0x8 | (bits & 0x7));
  }

  void InstructionBuilder::data_type(uint8_t bits) {
    write(3, bits & 0x3);
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
    write(2, ARG_REG_OFF);
    write(8, reg);
    write(24, *(uint32_t *) &offset & 0xffffff);
  }
}
