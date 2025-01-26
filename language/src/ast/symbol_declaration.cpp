#include "symbol_declaration.hpp"
#include "shell.hpp"
#include "symbol/variable.hpp"
#include "context.hpp"
#include "assembly/directive.hpp"
#include "symbol/registry.hpp"

std::ostream &lang::ast::SymbolDeclarationNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "let " << token_.image << ": ";
  return type_.print_code(os, 0);
}

std::ostream &lang::ast::SymbolDeclarationNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << token_.image << SHELL_RESET << ": " << SHELL_CYAN;
  type_.print_code(os, 0);
  return os << SHELL_RESET;
}

bool lang::ast::SymbolDeclarationNode::collate_registry(message::List& messages, symbol::Registry& registry) {
  auto maybe_id = symbol::create_variable(registry, arg_ ? symbol::Category::Argument : symbol::Category::Ordinary, token_, type_, messages);
  if (maybe_id.has_value()) {
    id_ = maybe_id.value();
    return true;
  }
  return false;
}

bool lang::ast::SymbolDeclarationNode::process(lang::Context& ctx) {
  return true;
}
