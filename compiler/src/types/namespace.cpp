#include "namespace.hpp"

std::ostream &lang::type::NamespaceNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << "namespace";
}

constants::inst::datatype::dt lang::type::NamespaceNode::get_asm_datatype() const {
  // should not be called
  throw std::runtime_error("namespace::get_asm_datatype should never be called");
}

lang::type::NamespaceNode lang::type::name_space;
