#include "int.hpp"

std::ostream &lang::ast::type::IntNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << (signed_ ? "int" : "uint") << width_ * 8;
}

std::string lang::ast::type::IntNode::name() const {
  return (signed_ ? "int" : "uint") + std::to_string(width_ * 8);
}

constants::inst::datatype::dt lang::ast::type::IntNode::get_asm_datatype() const {
  using namespace constants::inst::datatype;
  return width_ == 8
    ? (signed_ ? s64 : u64)
    : (signed_ ? s32 : u32);
}

std::string lang::ast::type::IntNode::to_label() const {
  return name();
}

lang::ast::type::IntNode
  lang::ast::type::uint8(1, false),
  lang::ast::type::int8(1, true),
  lang::ast::type::uint16(2, false),
  lang::ast::type::int16(2, true),
  lang::ast::type::uint32(4, false),
  lang::ast::type::int32(4, true),
  lang::ast::type::uint64(8, false),
  lang::ast::type::int64(8, true);
