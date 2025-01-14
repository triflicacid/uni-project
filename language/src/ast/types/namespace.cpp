#include "namespace.hpp"

std::ostream &lang::ast::type::NamespaceNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << "namespace";
}

lang::ast::type::NamespaceNode lang::ast::type::name_space;
