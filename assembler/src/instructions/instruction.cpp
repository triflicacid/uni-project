#include <iostream>
#include <utility>

#include "instruction.hpp"
#include "signature.hpp"

#include "constants.hpp"

namespace assembler::instruction {
  void Instruction::debug_print(std::ostream &os) const {
    os << "Signature '" << signature->mnemonic;

    for (ArgumentType type: signature->arguments[overload]) {
      os << ' ' << Argument::type_to_string(type);
    }

    os << "'; Opcode = 0x" << std::hex << (int) signature->opcode << std::dec << "; "
       << args.size() << " argument(s)" << std::endl;

    for (Argument arg: args) {
      os << "\t- ";
      arg.debug_print(os);
      os << std::endl;
    }
  }

  void Instruction::print(std::ostream &os) const {
    os << signature->mnemonic;

    if (test & 0x80) {
      os << constants::cmp::to_string(static_cast<constants::cmp::flag>(test & constants::inst::cmp_mask));
    }

    // TODO correct formatting for `cvt` instruction
    for (const auto &datatype: datatypes) {
      os << "." << constants::inst::datatype::to_string(datatype);
    }

    for (int i = 0; i < args.size(); i++) {
      os << " ";
      args[i].print(os);
      if (i + 1 < args.size()) os << ",";
    }
  }

  Instruction::Instruction(const Signature *signature, std::deque<Argument> arguments) : signature(signature),
                                                                                         args(std::move(arguments)),
                                                                                         overload(0), test(0x0) {}

  void Instruction::set_conditional_test(constants::cmp::flag mask) {
    test = 0x80 | static_cast<uint8_t>(mask); // indicate test is provided
  }

  void Instruction::add_datatype_specifier(constants::inst::datatype::dt mask) {
    datatypes.push_back(mask);
  }

  void Instruction::offset_addresses(uint16_t offset) {
    for (auto &arg: args) {
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

    // add datatype specifiers?
    for (auto &datatype: datatypes) {
      builder.data_type(datatype);
    }

    // ensure a datatype is added if expected
    if (signature->expect_datatype && datatypes.empty()) {
      builder.data_type(0x0);
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
        default:;
      }

      switch (arg.get_type()) {
        case ArgumentType::Address:
          builder.arg_addr(arg.get_data());
          break;
        case ArgumentType::Byte:
          builder.write(8, arg.get_data());
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
        default:;
      }
    }

    return builder.get();
  }

  std::set<std::string> Instruction::get_referenced_labels() const {
    std::set<std::string> labels;
    for (auto& arg : args) {
      if (arg.is_label()) {
        labels.insert(arg.get_label()->label);
      }
    }
    return labels;
  }

  void Instruction::replace_label(const std::string& label, uint32_t address, bool debug) {
    for (uint8_t i = 0; i < args.size(); i++) {
      auto &arg = args[i];

      if (arg.is_label() && arg.get_label()->label == label) {
        if (debug)
          std::cout << "Replace label " << label << " with address 0x" << std::hex << address
                    << std::dec << std::endl;
        arg.update(signature->arguments[overload][i] == instruction::ArgumentType::Address || arg.get_label()->is_addr
                   ? instruction::ArgumentType::Address
                   : instruction::ArgumentType::Immediate, address + arg.get_label()->offset);
      }
    }
  }

  void InstructionBuilder::opcode(uint8_t opcode) {
    write(constants::inst::op_size, opcode & constants::inst::op_mask);
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
    write(4, static_cast<uint64_t>(constants::cmp::na));
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
        write(2, constants::inst::arg::reg);
        write(32, reg);
        break;
      case NextArgument::AsAddress:
        write(1, constants::inst::arg::reg_indirect & 0x1);
        write(32, reg);
        break;
    }

    m_next = NextArgument::None;
  }

  void InstructionBuilder::arg_imm(uint32_t imm) {
    write(2, constants::inst::arg::imm);
    write(32, imm);

    m_next = NextArgument::None;
  }

  void InstructionBuilder::arg_addr(uint32_t addr) {
    switch (m_next) {
      case NextArgument::None:
        return;
      case NextArgument::AsValue:
        write(2, constants::inst::arg::mem);
        break;
      case NextArgument::AsAddress:
        write(1, constants::inst::arg::mem & 0x1);
        break;
    }

    write(32, addr);

    m_next = NextArgument::None;
  }

  void InstructionBuilder::arg_reg_indirect(uint8_t reg, int16_t offset) {
    switch (m_next) {
      case NextArgument::None:
        return;
      case NextArgument::AsValue:
        write(2, constants::inst::arg::reg_indirect);
        break;
      case NextArgument::AsAddress:
        write(1, constants::inst::arg::reg_indirect & 0x1);
        break;
    }

    write(8, reg);
    write(16, *(uint16_t*)&offset);

    m_next = NextArgument::None;
  }
}
