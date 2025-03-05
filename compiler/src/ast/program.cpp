#include "program.hpp"
#include "context.hpp"
#include "symbol/registry.hpp"
#include "config.hpp"
#include "message_helper.hpp"
#include "assembly/create.hpp"

void lang::ast::ProgramNode::add(std::unique_ptr<Node> ast_node) {
  token_end(ast_node->token_end());
  lines_.push_back(std::move(ast_node));
}

void lang::ast::ProgramNode::add(std::deque<std::unique_ptr<Node>> ast_nodes) {
  if (ast_nodes.empty()) return;
  token_end(ast_nodes.back()->token_end());

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
  Node::print_tree(os, indent_level);
  for (auto& line : lines_) {
    os << std::endl;
    line->print_tree(os, indent_level + 1);
  }
  return os;
}

bool lang::ast::ProgramNode::process(lang::Context& ctx) {
  // phase 1:
  // collate registries of children - this allows forward use of symbols
  symbol::Registry registry;
  for (auto& line : lines_) {
    if (!line->collate_registry(ctx.messages, registry))
      return false;
  }

  // add registry to our symbol table
  ctx.symbols.insert(registry);

  // phase 2 & 3:
  // process each of our children
  for (auto& line : lines_) {
    if (!line->process(ctx) || !line->resolve(ctx)) return false;
  }

  return true;
}

bool lang::ast::ProgramNode::generate_code(lang::Context& ctx) {
  // phase 4:
  // generate code for each child
  for (auto& line : lines_) {
    if (!line->generate_code(ctx)) return false;
  }

  // add 'exit' to ensure program exits OK
  ctx.program.current().add(assembly::create_exit());
  ctx.program.current().back().origin(token_end().loc);

  return true;
}
