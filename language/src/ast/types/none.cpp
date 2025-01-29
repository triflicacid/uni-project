#include "none.hpp"

std::ostream& lang::ast::type::NoneNode::print_code(std::ostream& os, unsigned int indent_level) const {
  return os << "none";
}

constants::inst::datatype::dt lang::ast::type::NoneNode::get_asm_datatype() const {
  throw std::runtime_error("none::get_asm_type should never be called");
}

lang::ast::type::NoneNode lang::ast::type::none;
