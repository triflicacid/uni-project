#include <unordered_map>
#include <memory>
#include "operator.hpp"
#include "ast/types/function.hpp"

static lang::ops::OperatorId current_id = 0;

lang::ops::Operator::Operator(std::string symbol, const lang::ast::type::FunctionNode& type)
  : symbol_(std::move(symbol)), type_(type), id_(current_id++) {}

static std::unordered_map<lang::ops::OperatorId, std::unique_ptr<lang::ops::Operator>> operators;

std::deque<std::reference_wrapper<const lang::ops::Operator>> lang::ops::get(const std::string symbol) {
  std::deque<std::reference_wrapper<const Operator>> matches;
  for (auto& [id, op] : operators) {
    if (op->symbol() == symbol)
      matches.push_back(*op);
  }

  return matches;
}

std::optional<std::reference_wrapper<const lang::ops::Operator>> lang::ops::get(const std::string symbol, const lang::ast::type::FunctionNode& type) {
  // search to see if a matching operator exists
  for (auto& [id, op] : operators) {
    if (op->symbol() == symbol && op->type().id() == type.id()) {
      return *op;
    }
  }

  return {};
}

void lang::ops::store_operator(std::unique_ptr<Operator> op) {
  const OperatorId id = op->id();
  operators.insert({id, std::move(op)});
}
