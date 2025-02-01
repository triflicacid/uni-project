#include "namespace.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "symbol/namespace.hpp"
#include "config.hpp"

std::ostream& lang::ast::NamespaceNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "namespace " << token_.image << " {";
  for (auto& line : lines_) {
    os << std::endl;
    indent(os, indent_level + 1);
    line->print_code(os, indent_level + 1);
  }
  os << std::endl;
  indent(os, indent_level);
  return os << "}";
}

std::ostream& lang::ast::NamespaceNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level) << SHELL_GREEN << token_.image << SHELL_RESET;
  for (auto& line : lines_) {
    os << std::endl;
    line->print_tree(os, indent_level + 1);
  }
  return os;
}

bool lang::ast::NamespaceNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  // check if our name already exists
  if (auto& others = registry.get(token_.image); !others.empty()) {
    auto msg = token_.generate_message(message::Error);
    msg->get() << "symbol " << token_.image << " is already bound - a namespace must be unique";
    messages.add(std::move(msg));

    // report all existing symbols to the user
    for (symbol::SymbolId id : others) {
      const symbol::Symbol& symbol = registry.get(id);
      msg = symbol.token().generate_message(message::Note);
      msg->get() << "previously bound here";
      messages.add(std::move(msg));
    }

    return false;
  }

  // insert into registry as a Namespace symbol
  auto symbol = std::make_unique<symbol::Namespace>(token_);
  id_ = symbol->id();
  registry.insert(std::move(symbol));

  // collate lines in a separate registry
  registry_ = std::make_unique<symbol::Registry>();
  for (auto& line : lines_) {
    if (!line->collate_registry(messages, *registry_)) return false;
  }

  // add us as a parent to all collated symbols
  auto& ns = registry.get(id_);
  for (auto& [symbolId, symbol] : *registry_) {
    symbol->set_parent(ns);
  }

  return true;
}

bool lang::ast::NamespaceNode::process(lang::Context& ctx) {
  assert(registry_ != nullptr);

  // record nested structure of namespace
  ctx.symbols.push_path(id_);

  // insert into symbol table
  ctx.symbols.insert(*registry_);

  // process children
  for (auto& line : lines_) {
    if (!line->process(ctx)) return false;
  }

  ctx.symbols.pop_path();
  return true;
}

void lang::ast::NamespaceNode::add(std::unique_ptr<Node> ast_node) {
  lines_.push_back(std::move(ast_node));
}

void lang::ast::NamespaceNode::add(std::deque<std::unique_ptr<Node>> ast_nodes) {
  while (!ast_nodes.empty()) {
    lines_.push_back(std::move(ast_nodes.front()));
    ast_nodes.pop_front();
  }
}
