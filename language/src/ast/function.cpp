#include "function.hpp"
#include "types/function.hpp"
#include "shell.hpp"

std::ostream& lang::ast::FunctionNode::print_code(std::ostream& os, unsigned int indent_level) const {
  // print comma-separated bracketed list of parameter types
  indent(os, indent_level) << "func " << token_.image << "(";
  for (int i = 0; i < params_.size(); i++) {
    os << params_[i]->token().image << ": ";
    type_->arg(i).print_code(os, 0);
    if (i < params_.size() - 1) os << ", ";
  }
  os << ")";

  // if provided, print return type after an arrow
  if (auto returns = type_->returns(); returns.has_value()) {
    os << " -> ";
   returns.value().get().print_code(os, 0);
  }

  // print body
  if (body_.has_value()) {
    os << " {" << std::endl;
    body_.value()->print_code(os, indent_level + 1);
    return indent(os, indent_level) << "}" << std::endl;
  }

  return os << ";" << std::endl;
}

std::ostream& lang::ast::FunctionNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level)  << SHELL_GREEN << token_.image << SHELL_RESET << std::endl;

  // print arguments
  for (auto& param : params_) {
    indent(os, indent_level);
    param->print_tree(os, indent_level + 1) << std::endl;
  }

  // print body
  if (body_.has_value()) {
    indent(os, indent_level);
    body_.value()->print_tree(os, indent_level + 1) << std::endl;
  }

  return os;
}

bool lang::ast::FunctionNode::process(lang::Context& ctx) {
  // TODO
  return true;
}
