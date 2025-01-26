#include <cassert>
#include "operator.hpp"
#include "shell.hpp"

const lang::ast::type::Node &lang::ast::expr::OperatorNode::type() const {
  // if type_ is not determined, error
  assert(type_.has_value());
  return type_.value();
}

std::ostream &lang::ast::expr::BinaryOperatorNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "(";
  lhs_->print_code(os, indent_level);
  os << " " << token_.image << " ";
  rhs_->print_code(os, indent_level);
  return os << ")";
}

std::ostream &lang::ast::expr::BinaryOperatorNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << " " SHELL_GREEN << token_.image << std::endl;

  indent(os, indent_level + 1);
  lhs_->print_tree(os, indent_level + 1) << std::endl;

  indent(os, indent_level + 1);
  rhs_->print_tree(os, indent_level + 1);

  return os;
}

std::ostream &lang::ast::expr::UnaryOperatorNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "(" << token_.image << " ";
  expr_->print_code(os, indent_level);
  return os << ")";
}

std::ostream &lang::ast::expr::UnaryOperatorNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << " " SHELL_GREEN << token_.image << std::endl;

  indent(os, indent_level + 1);
  expr_->print_tree(os, indent_level + 1);

  return os;
}
