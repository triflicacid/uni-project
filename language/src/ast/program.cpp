#include "program.hpp"
#include "context.hpp"
#include "symbol/registry.hpp"
#include "config.hpp"
#include "lint.hpp"

void lang::ast::ProgramNode::add(std::unique_ptr<Node> ast_node) {
  lines_.push_back(std::move(ast_node));
}

void lang::ast::ProgramNode::add(std::deque<std::unique_ptr<Node>> ast_nodes) {
  while (!ast_nodes.empty()) {
    lines_.push_back(std::move(ast_nodes.front()));
    ast_nodes.pop_front();
  }
}

std::ostream& lang::ast::ProgramNode::print_code(std::ostream& os, unsigned int indent_level) const {
  for (auto& line : lines_) {
    line->print_code(os, 0);
    os << std::endl;
  }
  return os;
}

std::ostream& lang::ast::ProgramNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level) << std::endl;
  for (auto& line : lines_) {
    indent(os, indent_level);
    line->print_tree(os, indent_level + 1) << std::endl;
  }
  return os;
}

bool lang::ast::ProgramNode::process(lang::Context& ctx) {
  // collate registries of children - this allows forward use of symbols
  symbol::Registry registry;
  for (auto& line : lines_) {
    if (!line->collate_registry(ctx.messages, registry))
      return false;
  }

  // add registry to our symbol table
  ctx.symbols.insert(registry);

  // finally, process each of our children
  for (auto& line : lines_) {
    if (!line->process(ctx))  {
      return false;
    }
  }

  // if linting, check for unused variables
  lint::check_local_scope(ctx.symbols, ctx.messages);

  return true;
}

