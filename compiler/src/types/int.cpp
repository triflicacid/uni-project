#include "int.hpp"

std::ostream &lang::type::IntNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << (signed_ ? "i" : "u") << width_ * 8;
}

std::string lang::type::IntNode::node_name() const {
  return (signed_ ? "i" : "u") + std::to_string(width_ * 8);
}

constants::inst::datatype::dt lang::type::IntNode::get_asm_datatype() const {
  using namespace constants::inst::datatype;
  return width_ == 8
    ? (signed_ ? s64 : u64)
    : (signed_ ? s32 : u32);
}

std::string lang::type::IntNode::to_label() const {
  return node_name();
}

lang::type::IntNode
  lang::type::uint8(1, false),
  lang::type::int8(1, true),
  lang::type::uint16(2, false),
  lang::type::int16(2, true),
  lang::type::uint32(4, false),
  lang::type::int32(4, true),
  lang::type::uint64(8, false),
  lang::type::int64(8, true);
