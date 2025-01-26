#include "function.hpp"
#include "graph.hpp"

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

std::string lang::ast::type::FunctionNode::to_label() const {
  std::stringstream label;
  for (auto& param : parameters_) {
    label << "_" << param.get().to_label();
  }
  return label.str();
}

bool lang::ast::type::FunctionNode::is_subtype(const lang::ast::type::FunctionNode& parent) const {
  // check if functions have the same number of arguments
  if (parameters_.size() != parent.parameters_.size()) return false;

  // if any parameter is NOT a subtype, the entire signature is not
  for (int i = 0; i < parameters_.size(); i++) {
    if (!graph.is_subtype(parameters_[i].get().id(), parent.parameters_[i].get().id())) {
      return false;
    }
  }

  return true;
}

const lang::ast::type::FunctionNode&
lang::ast::type::FunctionNode::create(const std::deque<std::reference_wrapper<const Node>>& parameters, const std::optional<std::reference_wrapper<const Node>>& returns) {
  // iterate through all registered types, check if already exists
  for (auto& [id, type] : graph) {
    if (auto func_type = type.get().get_func()) {
      // check if all parameter types match
      if (func_type->args() != parameters.size()) continue;
      bool are_equal = true;
      for (int i = 0; i < parameters.size(); i++) {
        if ((are_equal = func_type->arg(i).id() == parameters[i].get().id())) break;
      }
      if (are_equal) return *func_type;
    }
  }

  // otherwise, create a new type
  auto type = std::make_unique<FunctionNode>(parameters, std::move(returns));
  const TypeId id = type->id();
  graph.insert(std::move(type));
  return static_cast<FunctionNode&>(graph.get(id));
}
