#include "bool.hpp"

lang::ast::type::BoolNode lang::ast::type::boolean;

std::ostream& lang::ast::type::BoolNode::print_code(std::ostream& os, unsigned int indent_level) const {
  return os << "bool";
}

constants::inst::datatype::dt lang::ast::type::BoolNode::get_asm_datatype() const {
  return constants::inst::datatype::u32;
}
