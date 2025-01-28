#include "literal.hpp"
#include "ast/types/int.hpp"
#include "uint64.hpp"
#include "ast/types/float.hpp"
#include "shell.hpp"
#include "context.hpp"

std::ostream &lang::ast::expr::LiteralNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "(";
  type_.print_code(os);
  os << ") " << to_string();
  return os;
}

std::string lang::ast::expr::LiteralNode::to_string() const {
  if (auto int_ = type_.get_int()) {
    return int_->is_signed()
      ? std::to_string(uint64::to_int64(get()))
      : std::to_string(uint64::to_uint64(get()));
  } else {
    auto float_ = type_.get_float();
    return float_->is_double()
      ? std::to_string(uint64::to_double(get()))
      : std::to_string(uint64::to_float(get()));
  }
}

std::ostream &lang::ast::expr::LiteralNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << " " SHELL_GREEN << to_string() << SHELL_RESET ": " SHELL_CYAN;
  return type_.print_code(os, 0) << SHELL_RESET;
}

bool lang::ast::expr::LiteralNode::process(lang::Context& ctx) {
  return true;
}

bool lang::ast::expr::LiteralNode::load(lang::Context& ctx) const {
  ctx.reg_alloc_manager.find(*this);
  return true;
}

