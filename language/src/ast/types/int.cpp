#include "int.hpp"

std::ostream &lang::ast::type::IntNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << (signed_ ? "int" : "uint") << width_ * 8;
}

std::string lang::ast::type::IntNode::name() const {
  return "Type::" + std::string(signed_ ? "Int" : "Uint") + std::to_string(width_ * 8);
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
