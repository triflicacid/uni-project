#include "literal.hpp"
#include "ast/types/int.hpp"
#include "uint64.hpp"
#include "ast/types/float.hpp"
#include "shell.hpp"
#include "context.hpp"

std::ostream &lang::ast::expr::LiteralNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "(";
  lit_.type().print_code(os);
  os << ") " << lit_.to_string();
  return os;
}

std::ostream &lang::ast::expr::LiteralNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << " " SHELL_GREEN << lit_.to_string() << SHELL_RESET ": " SHELL_CYAN;
  return lit_.type().print_code(os, 0) << SHELL_RESET;
}

bool lang::ast::expr::LiteralNode::process(lang::Context& ctx) {
  return true;
}

bool lang::ast::expr::LiteralNode::load(lang::Context& ctx) const {
  ctx.reg_alloc_manager.find(lit_);
  return true;
}
