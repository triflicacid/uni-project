#include "float.hpp"

std::string lang::ast::type::FloatNode::name() const {
  return double_ ? "double" : "float";
}

std::ostream &lang::ast::type::FloatNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << (double_ ? "double" : "float");
}

constants::inst::datatype::dt lang::ast::type::FloatNode::get_asm_datatype() const {
  return double_
    ? constants::inst::datatype::dbl
    : constants::inst::datatype::flt;
}

std::string lang::ast::type::FloatNode::to_label() const {
  return name();
}

lang::ast::type::FloatNode
  lang::ast::type::float32(false),
  lang::ast::type::float64(true);
