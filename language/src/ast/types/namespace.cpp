#include "namespace.hpp"

std::ostream &lang::ast::type::NamespaceNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << "namespace";
}

constants::inst::datatype::dt lang::ast::type::NamespaceNode::get_asm_datatype() const {
  // should not be called
  throw std::runtime_error("namespace::get_asm_datatype should never be called");
}

lang::ast::type::NamespaceNode lang::ast::type::name_space;
