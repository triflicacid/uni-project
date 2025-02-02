#include "literal.hpp"
#include "ast/types/int.hpp"
#include "ast/types/float.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "value/literal.hpp"

std::ostream &lang::ast::expr::LiteralNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << lit_.to_string();
}

std::ostream &lang::ast::expr::LiteralNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << lit_.to_string() << SHELL_RESET ": " SHELL_CYAN;
  return lit_.type().print_code(os, 0) << SHELL_RESET;
}

bool lang::ast::expr::LiteralNode::process(lang::Context& ctx) {
  return true;
}

std::unique_ptr<lang::value::Value> lang::ast::expr::LiteralNode::get_value(lang::Context& ctx) const {
  return std::make_unique<value::Literal>(lit_); // uninitialised literal (r)value
}

std::unique_ptr<lang::value::Value> lang::ast::expr::LiteralNode::load(lang::Context& ctx) const {
  const memory::Ref ref = ctx.reg_alloc_manager.find(lit_);
  return std::make_unique<value::Literal>(lit_, value::Options{.rvalue=ref});
}
