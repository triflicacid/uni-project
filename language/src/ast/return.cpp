#include "return.hpp"

std::ostream& lang::ast::ReturnNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "return";
  if (expr_.has_value()) {
    os << " ";
    expr_.value()->print_code(os, indent_level);
  }
  return os;
}

std::ostream& lang::ast::ReturnNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  if (expr_.has_value()) {
    os << std::endl;
    indent(os, indent_level + 1);
    expr_.value()->print_tree(os, indent_level + 1);
  }
  return os;
}
