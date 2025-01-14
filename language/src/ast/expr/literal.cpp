#include "literal.hpp"
#include "ast/types/int.hpp"
#include "uint64.hpp"
#include "ast/types/float.hpp"
#include "shell.hpp"

std::string lang::ast::expr::LiteralNode::name() const {
  return "literal{" + type_.name() + "}";
}

std::ostream &lang::ast::expr::LiteralNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "(";
  type_.print_code(os);
  os << ") " << to_string();
  return os;
}

std::string lang::ast::expr::LiteralNode::to_string() const {
  if (type_.is_int()) {
    auto& int_ = static_cast<const ast::type::IntNode&>(type_);
    if (int_.is_signed()) {
      return std::to_string(uint64::to_int64(get()));
    } else {
      return std::to_string(uint64::to_uint64(get()));
    }
  } else {
    auto& float_ = static_cast<const ast::type::FloatNode&>(type_);
    if (float_.is_double()) {
      return std::to_string(uint64::to_double(get()));
    } else {
      return std::to_string(uint64::to_float(get()));
    }
  }
}

std::ostream &lang::ast::expr::LiteralNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  return os << " " SHELL_GREEN << to_string() << SHELL_RESET;
}

