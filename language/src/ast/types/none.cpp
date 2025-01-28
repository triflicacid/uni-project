#include "none.hpp"

std::ostream& lang::ast::type::NoneNode::print_code(std::ostream& os, unsigned int indent_level) const {
  return os << "none";
}

lang::ast::type::NoneNode lang::ast::type::none;
