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

bool lang::ast::SymbolDeclarationNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  // check if the symbol exists, we need additional guarding logic if it does
  const std::string name = token_.image;
  auto& others = registry.get(name);
  if (!others.empty()) {
    // if we are a function, check if this overload already exists
    // otherwise, we are shadowing
    if (auto func_type = type_.get_func()) {
      // TODO
      throw std::runtime_error("declaring function types is not implemented");
    } else {
      // if not empty, should only contain one element
      registry.remove(others.front());
    }
  }

  // TODO add parent namespace

  // create Symbol object & register in the symbol table
  auto symbol = std::make_unique<symbol::Variable>(token_, type_);
  registry.insert(std::move(symbol));

  return true;
}

bool lang::ast::SymbolDeclarationNode::process(lang::Context& ctx) {
  return true;
}
