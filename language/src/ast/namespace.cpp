#include "namespace.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "symbol/namespace.hpp"
#include "config.hpp"

std::ostream& lang::ast::NamespaceNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "namespace " << token_.image << " ";
  return body_->print_code(os, indent_level);
}

std::ostream& lang::ast::NamespaceNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level) << SHELL_GREEN << token_.image << SHELL_RESET << std::endl;
  indent(os, indent_level);
  return body_->print_tree(os, indent_level + 1);
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
  const symbol::SymbolId ns_id = symbol->id();
  registry.insert(std::move(symbol));

  // collate block body as normal
  if (!body_->collate_registry(messages, registry))
    return false;

  // lint: is this namespace empty?
  if (conf::lint && body_->registry_->empty()) {
    auto msg = token_.generate_message(conf::lint_level);
    msg->get() << "namespace " << token_.image << " is empty";
    messages.add(std::move(msg));
  }

  // add us as a parent to this symbol
  auto& ns = registry.get(ns_id);
  for (auto& [symbolId, symbol] : *body_->registry_) {
    symbol->set_parent(ns);
  }

  return true;
}

bool lang::ast::NamespaceNode::process(lang::Context& ctx) {
  return body_->process(ctx);
}
