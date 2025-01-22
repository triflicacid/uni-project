#include "block.hpp"
#include "context.hpp"

void lang::ast::BlockNode::add(std::unique_ptr<Node> ast_node) {
  lines_.push_back(std::move(ast_node));
}

std::ostream& lang::ast::BlockNode::print_code(std::ostream& os, unsigned int indent_level) const {
  for (auto& line : lines_) {
    indent(os, indent_level);
    line->print_code(os, indent_level + 1);
    os << std::endl;
  }
  return os;
}

std::ostream& lang::ast::BlockNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level) << std::endl;
  for (auto& line : lines_) {
    indent(os, indent_level);
    line->print_tree(os, indent_level + 1) << std::endl;
  }
  return os;
}

bool lang::ast::BlockNode::process(lang::Context& ctx) {
  if (scope_) ctx.symbols.push();
  for (auto& line : lines_) {
    if (!line->process(ctx))  {
      return false;
    }
  }
  if (scope_) ctx.symbols.pop();
  return true;
}
