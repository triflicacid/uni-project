#include "node.hpp"
#include "shell.hpp"
#include "expr/literal.hpp"

std::ostream &lang::ast::indent(std::ostream &os, unsigned int level) {
  return os << std::string(level * 2, ' ');
}

std::ostream &lang::ast::Node::print_tree(std::ostream &os, unsigned int indent_level) const {
  indent(os, indent_level);
  return os << SHELL_LIGHT_PURPLE << name() << SHELL_RESET " @<" SHELL_BROWN
      << token_.source.path() << ":" << token_.source.line() << ":" << token_.source.column() << SHELL_RESET "> ";
}
