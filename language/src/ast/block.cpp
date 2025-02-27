#include "block.hpp"
#include "context.hpp"
#include "symbol/registry.hpp"
#include "config.hpp"
#include "message_helper.hpp"
#include "value/future.hpp"
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

lang::value::Value& lang::ast::BlockNode::value() const {
  if (returns_ && !lines_.empty()) return lines_.back()->value();
  return Node::value();
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

bool lang::ast::BlockNode::process(Context& ctx) {
  // if new scope, add new scope to symbol table & add local registry
  if (scope_) {
    assert(registry_ != nullptr);
    ctx.symbols.push();
    ctx.symbols.insert(*registry_);
    registry_ = nullptr;
  }

  // warn users about empty code blocks
  if (lines_.empty() && lang::conf::lint) {
    auto msg = generate_message(lang::conf::lint_level);
    msg->get() << "empty code block";
    ctx.messages.add(std::move(msg));
    if (lang::conf::lint_level == message::Error) return false;
  }

  // process our children
  for (int i = 0; i < lines_.size(); i++) {
    Node& line = *lines_[i];

    // if previous line returned, warn on unreachable code
    if (i > 0 && lines_[i - 1]->always_returns()) {
      auto msg = line.generate_message(message::Warning);
      msg->get() << "unreachable code detected; this code will never be executed";
      ctx.messages.add(std::move(msg));
      break; // do not generate code further
    }

    // else, process this line
    if (!line.process(ctx)) return false;
  }

  // remove local scope if necessary
  if (scope_) {
    ctx.symbols.pop();
  }

  return true;
}

bool lang::ast::BlockNode::resolve(lang::Context& ctx) {
  for (int i = 0; i < lines_.size(); i++) {
    Node& line = *lines_[i];
    if (i > 0 && lines_[i - 1]->always_returns()) {
      break;
    }

    if (!line.resolve(ctx)) return false;
  }

  return true;
}

bool lang::ast::BlockNode::generate_code(lang::Context& ctx) {
  // forwards our target to our last child
  if (!lines_.empty() && target()) {
    lines_.back()->target(target()->get());
  }

  // generate code of our children
  for (int i = 0; i < lines_.size(); i++) {
    Node& line = *lines_[i];
    if (i > 0 && lines_[i - 1]->always_returns()) {
      break;
    }

    if (!line.generate_code(ctx)) return false;
  }

  return true;
}

bool lang::ast::BlockNode::always_returns() const {
  // we return if any of our children does
  for (auto& line : lines_) {
    if (line->always_returns()) return true;
  }
  return false;
}
