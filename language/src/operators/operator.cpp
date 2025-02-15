#include <unordered_map>
#include <memory>
#include "operator.hpp"
#include "ast/types/function.hpp"

static lang::ops::OperatorId current_id = 0;

lang::ops::Operator::Operator(std::string symbol, const lang::ast::type::FunctionNode& type)
  : op_(std::move(symbol)), type_(type), id_(current_id++) {}

std::ostream& lang::ops::Operator::print_code(std::ostream& os) const {
  os << "operator" << op();
  type_.print_code(os);
  return os;
}

static std::unordered_map<lang::ops::OperatorId, std::unique_ptr<lang::ops::Operator>> operators;

std::deque<std::reference_wrapper<const lang::ops::Operator>> lang::ops::get(const std::string& symbol) {
  std::deque<std::reference_wrapper<const Operator>> matches;
  for (auto& [id, op] : operators) {
    if (op->op() == symbol)
      matches.push_back(*op);
  }

  return matches;
}

std::optional<std::reference_wrapper<const lang::ops::Operator>> lang::ops::get(const std::string& symbol, const lang::ast::type::FunctionNode& type) {
  // search to see if a matching operator exists
  for (auto& [id, op] : operators) {
    if (op->op() == symbol && op->type() == type) {
      return *op;
    }
  }

  return {};
}

void lang::ops::store_operator(std::unique_ptr<Operator> op) {
  const OperatorId id = op->id();
  operators.insert({id, std::move(op)});
}

optional_ref<const lang::ops::Operator>
lang::ops::select_candidate(const std::string& symbol, const ast::type::FunctionNode& signature, const message::MessageGenerator& source, message::List& messages) {
  // search for a matching operator
  auto options = ops::get(symbol);

  // extract types into an option list
  std::deque<std::reference_wrapper<const ast::type::FunctionNode>> candidates;
  for (auto& op : options) {
    candidates.push_back(op.get().type());
  }

  // filter to viable candidates
  candidates = signature.filter_candidates(candidates);

  // if only one candidate remaining: this is us! use signature + name to find operator object
  if (candidates.size() == 1) {
    return ops::get(symbol, candidates.front());
  }

  // otherwise, if candidates is non-empty, we can narrow options down for message
  if (!candidates.empty()) {
    options.clear();
    for (auto& candidate : candidates) {
      options.push_back(ops::get(symbol, candidate).value());
    }
  }

  // error to user, reporting our candidate list
  std::unique_ptr<message::BasicMessage> msg = source.generate_message(message::Error);
  msg->get() << "unable to resolve a suitable candidate for operator" << symbol << "(";
  for (int i = 0; i < signature.args(); i++) {
    signature.arg(i).print_code(msg->get());
    if (i + 1 < signature.args()) msg->get() << ", ";
  }
  msg->get() << ")";
  messages.add(std::move(msg));

  for (auto& option : options) {
    msg = std::make_unique<message::BasicMessage>(message::Note);
    msg->get() << "candidate: ";
    option.get().print_code(msg->get());
    messages.add(std::move(msg));
  }

  return {};
}
