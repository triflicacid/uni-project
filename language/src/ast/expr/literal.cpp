#include "literal.hpp"
#include "ast/types/int.hpp"
#include "ast/types/float.hpp"
#include "shell.hpp"
#include "context.hpp"

std::ostream &lang::ast::LiteralNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << lit_.to_string();
}

std::ostream &lang::ast::LiteralNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << lit_.to_string() << SHELL_RESET ": " SHELL_CYAN;
  return lit_.type().print_code(os, 0) << SHELL_RESET;
}

bool lang::ast::LiteralNode::process(lang::Context& ctx) {
  value_ = value::rvalue(lit_.type());
  return true;
}

bool lang::ast::LiteralNode::resolve_rvalue(Context& ctx) {
  const memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(lit_);
  value_->rvalue(std::make_unique<value::Literal>(lit_, ref));
  return true;
}
