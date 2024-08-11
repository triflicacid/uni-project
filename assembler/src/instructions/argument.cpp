#include "argument.hpp"

extern "C" {
#include "util.h"
}

#include <string>
#include <iomanip>

namespace assembler::instruction {
  void Argument::destroy() {
    if (m_data == 0x00) return;

    if (m_type == ArgumentType::Label) {
      delete (std::string *) m_data;
    } else if (m_type == ArgumentType::RegisterIndirect) {
      delete (ArgumentRegisterIndirect *) m_data;
    }

    m_data = 0x00;
  }

  Argument::Argument(int col, ArgumentType type, uint64_t data) {
    m_col = col;
    m_type = type;
    m_data = data;
  }

  bool Argument::type_match(const ArgumentType &target) {
    return type_accepts(target, m_type);
  }

  bool Argument::is_label() {
    return m_type == ArgumentType::Label;
  }

  void Argument::print(std::ostream &out) {
    switch (m_type) {
      case ArgumentType::Immediate:
        case ArgumentType::ImmediateValue:
        out << "immediate 0x" << std::hex << m_data << std::dec;
        break;
      case ArgumentType::Address:
        out << "address 0x" << std::hex << m_data << std::dec;
        break;
      case ArgumentType::Register:
      case ArgumentType::RegisterValue:
        out << "register " << m_data;
        break;
      case ArgumentType::RegisterIndirect:
        out << "register indirect " << get_reg_indirect()->offset << "(" << (int) get_reg_indirect()->reg << ")";
        break;
      case ArgumentType::Label:
        out << "label \"" << *get_label() << "\"";
        break;
      default:
        out << "type " << (int) m_type;
    }
  }

  void Argument::set_reg_indirect(uint8_t reg, int32_t offset) {
    destroy();

    auto data = new ArgumentRegisterIndirect;
    data->reg = reg;
    data->offset = offset;

    m_data = (uint64_t) data;
  }

  std::string Argument::type_to_string(const ArgumentType &type) {
    switch (type) {
      case ArgumentType::Immediate:
      case ArgumentType::ImmediateValue:
        return "<imm>";
      case ArgumentType::Address:
        return "<addr>";
      case ArgumentType::Register:
      case ArgumentType::RegisterValue:
        return "<reg>";
      case ArgumentType::RegisterIndirect:
        return "<addr: reg>";
      case ArgumentType::Label:
        return "<addr: label>";
      case ArgumentType::Value:
        return "<value>";
    }

    return "";
  }

  bool Argument::type_accepts(const ArgumentType &target, ArgumentType &type) {
    if (target == type) {
      return true;
    }

    if (type == ArgumentType::Immediate) {
      if (target == ArgumentType::Value || target == ArgumentType::ImmediateValue) {
        type = ArgumentType::ImmediateValue;
        return true;
      }

      return false;
    }

    if (type == ArgumentType::Register) {
      if (target == ArgumentType::Value) {
        type = ArgumentType::RegisterValue;
        return true;
      }

      if (target == ArgumentType::Address || target == ArgumentType::RegisterIndirect) {
        type = ArgumentType::RegisterIndirect;
        return true;
      }
    }

    if (type == ArgumentType::Label && (target == ArgumentType::Address || target == ArgumentType::Value)) {
      return true;
    }

    return false;
  }

  void Argument::set_label(const std::string &label) {
    destroy();

    m_type = ArgumentType::Label;
    auto ptr = new std::string(label);
    m_data = (uint64_t ) ptr;
  }

  void Argument::transform_label(uint64_t value) {
    if (m_type == ArgumentType::Label) {
      destroy();
      m_type = ArgumentType::Address;
      m_data = value;
    }
  }

  void Argument::update(ArgumentType type, uint64_t data) {
    destroy();

    m_type = type;
    m_data = data;
  }
}
