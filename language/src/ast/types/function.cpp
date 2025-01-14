#include "function.hpp"

std::ostream& lang::ast::type::FunctionNode::print_code(std::ostream& os, unsigned int indent_level) const {
  // print comma-separated bracketed list of parameter types
  os << "(";
  int i = 0;
  for (const auto& param : parameters_) {
    param.get().print_code(os, indent_level);
    if (++i < parameters_.size()) os << ", ";
  }
  os << ")";

  // if provided, print return type after an arrow
  if (returns_.has_value()) {
    os << " -> ";
    returns_.value().get().print_code(os, indent_level);
  }

  return os;
}
