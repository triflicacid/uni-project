#include "expr.hpp"

std::ostream& lang::ast::ExprNode::print_code(std::ostream& os, unsigned int indent_level) const {
  return expr_->print_code(os, indent_level);
}

std::ostream& lang::ast::ExprNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << std::endl;
  return expr_->print_tree(os, indent_level + 1);
}

bool lang::ast::ExprNode::process(lang::Context& ctx) {
  // process AND load to ensure entire expression is calculated
  // this is why we need a wrapper for expr::Node :)
  std::unique_ptr<value::Value> value;
  return expr_->process(ctx) && (value = expr_->get_value(ctx)) && expr_->resolve_rvalue(ctx, *value);
}
