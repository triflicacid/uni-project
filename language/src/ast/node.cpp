#include "node.hpp"
#include "shell.hpp"

std::ostream &lang::ast::indent(std::ostream &os, unsigned int level) {
  return os << std::string(level * 2, ' ');
}

std::ostream &lang::ast::Node::print_tree(std::ostream &os, unsigned int indent_level) const {
  indent(os, indent_level);
  return os << SHELL_LIGHT_PURPLE << name() << SHELL_RESET " @<" SHELL_BROWN
      << token_.source.path() << ":" << token_.source.line() << ":" << token_.source.column() << SHELL_RESET "> ";
}

bool lang::ast::Node::process(message::List &messages) {
  auto message = std::make_unique<message::Message>(message::Error, token_.source);
  message->get() << "AST '" << name() << "' ::process is unimplemented";
  messages.add(std::move(message));
  return false;
}
