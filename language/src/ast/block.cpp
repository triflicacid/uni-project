#include "block.hpp"
#include "context.hpp"
#include "symbol/registry.hpp"
#include "config.hpp"
#include "message_helper.hpp"
#include "value/symbol.hpp"
#include "optional_ref.hpp"

void lang::ast::BlockNode::add(std::unique_ptr<Node> ast_node) {
  lines_.push_back(std::move(ast_node));
}

void lang::ast::BlockNode::add(std::deque<std::unique_ptr<Node>> ast_nodes) {
  while (!ast_nodes.empty()) {
    lines_.push_back(std::move(ast_nodes.front()));
    ast_nodes.pop_front();
  }
}

std::ostream& lang::ast::BlockNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "{" << std::endl;
  for (auto& line : lines_) {
    indent(os, indent_level + 1);
    line->print_code(os, indent_level + 2);
    os << std::endl;
  }
  return indent(os, indent_level) << "}";
}

std::ostream& lang::ast::BlockNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  for (auto& line : lines_) {
    os << std::endl;
    line->print_tree(os, indent_level + 1);
  }
  return os;
}

bool lang::ast::BlockNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  symbol::Registry* local_registry = &registry;

  // create new registry representing local scope
  if (scope_) {
    registry_ = std::make_unique<symbol::Registry>();
    local_registry = registry_.get();
  }

  // collate children into new registry
  for (auto& line : lines_) {
    if (!line->collate_registry(messages, *local_registry))
      return false;
  }

  return true;
}

bool lang::ast::BlockNode::process(lang::Context& ctx) {
  // if new scope, add new scope to symbol table & add local registry
  if (scope_) {
    ctx.symbols.push();
    ctx.symbols.insert(*registry_);
    registry_ = nullptr;
  }

  // process our children
  for (auto& line : lines_) {
    if (!line->process(ctx) || !line->resolve_value(ctx)) return false;
  }

  // remove local scope if necessary
  if (scope_) {
    ctx.symbols.pop();
  }

  return true;
}

const lang::value::Value& lang::ast::BlockNode::value() const {
  if (returns_ && !lines_.empty()) return lines_.back()->value();
  return Node::value();
}
