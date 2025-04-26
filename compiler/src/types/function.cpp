#include <iostream>
#include "function.hpp"
#include "graph.hpp"

std::ostream& lang::type::FunctionNode::print_code(std::ostream& os, unsigned int indent_level) const {
  return print_code(os, true, indent_level);
}

std::ostream& lang::type::FunctionNode::print_code(std::ostream& os, bool print_return, unsigned int indent_level) const {
  // print comma-separated bracketed list of parameter types
  os << "(";
  int i = 0;
  for (const auto& param : parameters_) {
    param.get().print_code(os, indent_level);
    if (++i < parameters_.size()) os << ", ";
  }
  os << ")";

  // if desired, print return type after an arrow
  if (print_return) {
    os << " -> ";
    returns_.print_code(os, indent_level);
  }

  return os;
}

std::string lang::type::FunctionNode::to_label() const {
  std::stringstream label;
  for (auto& param : parameters_) {
    label << "_" << param.get().to_label();
  }
  return label.str();
}

const lang::type::FunctionNode&
lang::type::FunctionNode::create(const std::deque<std::reference_wrapper<const Node>>& parameters, optional_ref<const Node> returns) {
  // iterate through all registered types, check if already exists
  for (auto& [id, type] : graph) {
    if (auto func_type = type.get().get_func()) {
      // check if all parameter types match
      if (func_type->args() != parameters.size()) continue;
      bool are_equal = true;
      for (int i = 0; are_equal && i < parameters.size(); i++) {
        if (func_type->arg(i).id() != parameters[i].get().id()) {
          are_equal = false;
        }
      }
      if (are_equal && returns.has_value()) are_equal = returns.value().get() == func_type->returns();
      if (are_equal) return *func_type;
    }
  }

  // otherwise, create a new type
  auto type = std::make_unique<FunctionNode>(parameters, returns ? *returns : type::unit);
  const TypeId id = type->id();
  graph.insert(std::move(type));
  return static_cast<FunctionNode&>(graph.get(id));
}

std::deque<std::reference_wrapper<const lang::type::FunctionNode>> lang::type::FunctionNode::filter_candidates(
    const std::deque<std::reference_wrapper<const FunctionNode>>& options) const {
  return filter_candidates(parameters_, options);
}

std::deque<std::reference_wrapper<const lang::type::FunctionNode>>
lang::type::FunctionNode::filter_candidates(const std::deque<std::reference_wrapper<const Node>>& parameters, const std::deque<std::reference_wrapper<const FunctionNode>>& options) {
  std::deque<std::reference_wrapper<const FunctionNode>> candidates;
  int max_score = -1; // measure similarity between parameters and candidates[i], i.e., how many params are equal
  for (auto& option : options) {
    // we are a candidate if #args match and all types are equal or subtypes
    if (option.get().args() != parameters.size()) continue;
    bool is_candidate = true;
    int score = 0;
    for (int i = 0; is_candidate && i < parameters.size(); i++) {
      if (parameters[i].get() == option.get().arg(i)) score++;
      else if (!graph.is_subtype(parameters[i].get().id(), option.get().arg(i).id())) is_candidate = false;
    }
    // if perfect score, exact match, so ignore all others
    if (score == parameters.size()) return {option};
    // otherwise, add to candidate list
    if (is_candidate) {
      // if new high score, disregard all other candidates
      if (score > max_score) {
        candidates.clear();
        max_score = score;
      }
      // add candidate to shortlist but ONLY if it is not worse than the best candidate
      if (score == max_score) {
        candidates.push_back(option);
      }
    }
  }

  return candidates;
}

constants::inst::datatype::dt lang::type::FunctionNode::get_asm_datatype() const {
  return constants::inst::datatype::u64; // pointer type
}
