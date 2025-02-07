#include "function_call.hpp"

std::ostream& lang::ast::FunctionCallNode::print_code(std::ostream& os, unsigned int indent_level) const {
  subject_->print_code(os, indent_level);
  os << "(";
  for (auto& arg : args_) {
    arg->print_code(os);
    if (arg != args_.back()) os << ", ";
  }
  return os << ")";
}

std::ostream& lang::ast::FunctionCallNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << std::endl;
  subject_->print_tree(os, indent_level + 1);
  for (auto& arg : args_) {
    os << std::endl;
    arg->print_tree(os, indent_level + 1);
  }
  return os;
}
