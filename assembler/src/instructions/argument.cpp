#include "argument.hpp"

#include <parser.hpp>

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

  Argument::Argument(ArgumentType type, uint64_t data) {
    m_type = type;
    m_data = data;
  }

  bool Argument::type_match(const ArgumentType &target) {
    return type_accepts(target, m_type);
  }

  void Argument::debug_print(std::ostream &out) {
    switch (m_type) {
      case ArgumentType::Immediate:
      case ArgumentType::Byte:
      case ArgumentType::DecimalImmediate:
        out << "immediate 0x" << std::hex << m_data << std::dec;
        if (m_type == ArgumentType::DecimalImmediate)
          out << " (double " << *(double *) &m_data << ")";
        break;
      case ArgumentType::Address:
        out << "address 0x" << std::hex << m_data << std::dec;
        break;
      case ArgumentType::Register:
        out << "register $" << constants::registers::to_string(static_cast<constants::registers::reg>(m_data));
        break;
      case ArgumentType::RegisterIndirect:
        out << "register indirect " << get_reg_indirect()->offset << "($" << constants::registers::to_string(
            static_cast<constants::registers::reg>(get_reg_indirect()->reg)) << ")";
        break;
      case ArgumentType::Label: {
        auto* label = get_label();
        out << "label \"" << label->label << "\"";
        if (label->offset != 0) out << " + " << label->offset;
        break;
      }
      default:;
    }
  }

  void Argument::print(std::ostream &os) const {
    switch (m_type) {
      case ArgumentType::Immediate:
      case ArgumentType::Byte:
        os << "0x" << std::hex << m_data << std::dec;
        break;
      case ArgumentType::DecimalImmediate:
        os << *(double *) &m_data;
        break;
      case ArgumentType::Address:
        os << "(0x" << std::hex << m_data << std::dec << ")";
        break;
      case ArgumentType::Register:
        os << "$" << constants::registers::to_string(static_cast<constants::registers::reg>(m_data));
        break;
      case ArgumentType::RegisterIndirect:
        os << get_reg_indirect()->offset << "($" << constants::registers::to_string(
            static_cast<constants::registers::reg>(get_reg_indirect()->reg)) << ")";
        break;
      case ArgumentType::Label: {
        auto *label = get_label();
        os << label->label;
        if (label->offset != 0) os << "+" << label->offset;
        break;
      }
      default:;
    }
  }

  void Argument::set_reg_indirect(uint8_t reg, int32_t offset) {
    destroy();

    auto data = new ArgumentRegisterIndirect;
    data->reg = reg;
    data->offset = offset;

    m_data = (uint64_t) data;
    m_type = ArgumentType::RegisterIndirect;
  }

  std::string Argument::type_to_string(const ArgumentType &type) {
    switch (type) {
      case ArgumentType::Immediate:
      case ArgumentType::DecimalImmediate:
        return "<imm>";
      case ArgumentType::Byte:
        return "<imm: 8>";
      case ArgumentType::Address:
        return "<addr>";
      case ArgumentType::Label:
        return "<addr: label>";
      case ArgumentType::Value:
        return "<value>";
      case ArgumentType::Register:
        return "<reg>";
      case ArgumentType::RegisterIndirect:
        return "<addr: reg>";
      default:
        return "";
    }
  }

  bool Argument::type_accepts(const ArgumentType &target, ArgumentType &type) {
    if (target == type) {
      return true;
    }

    if (type == ArgumentType::Address) {
      return target == ArgumentType::Value;
    }

    if (type == ArgumentType::Immediate) {
      if (target == ArgumentType::Value) {
        type = ArgumentType::Immediate;
        return true;
      }

      return target == ArgumentType::DecimalImmediate;
    }

    if (type == ArgumentType::DecimalImmediate) {
      return target == ArgumentType::Value;
    }

    if (type == ArgumentType::Register) {
      return target == ArgumentType::Value;
    }

    if (type == ArgumentType::RegisterIndirect || type == ArgumentType::Label) {
      return target == ArgumentType::Address || target == ArgumentType::Value;
    }

    return false;
  }

  void Argument::update(ArgumentType type, uint64_t data) {
    destroy();
    m_type = type;
    m_data = data;
  }

  void Argument::set_label(const std::string &label, int offset, bool is_addr) {
    destroy();
    m_type = ArgumentType::Label;
    auto ptr = new ArgumentLabel(label, offset, is_addr);
    m_data = (uint64_t) ptr;
  }
}
