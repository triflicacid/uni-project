#include "symbol_reference.hpp"
#include "shell.hpp"

std::ostream &lang::ast::expr::SymbolReferenceNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << token_.image;
}

std::ostream &lang::ast::expr::SymbolReferenceNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  return os << " " SHELL_GREEN << token_.image << SHELL_RESET;
}
