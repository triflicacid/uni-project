#include "bool.hpp"

lang::type::BoolNode lang::type::boolean;

std::ostream& lang::type::BoolNode::print_code(std::ostream& os, unsigned int indent_level) const {
  return os << "bool";
}

constants::inst::datatype::dt lang::type::BoolNode::get_asm_datatype() const {
  return constants::inst::datatype::u32;
}
