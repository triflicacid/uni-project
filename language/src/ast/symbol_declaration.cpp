#include "symbol_declaration.hpp"
#include "shell.hpp"

std::ostream &lang::ast::SymbolDeclarationNode::print_code(std::ostream &os, unsigned int indent_level) const {
  type_.print_code(os, indent_level);
  return os << " " << token_.image;
}

std::ostream &lang::ast::SymbolDeclarationNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << token_.image << SHELL_RESET << ": " << SHELL_CYAN;
  type_.print_code(os, indent_level);
  return os << SHELL_RESET;
}
