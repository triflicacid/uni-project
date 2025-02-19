#include <unordered_map>
#include <memory>
#include "operator.hpp"
#include "ast/types/function.hpp"
#include "user_defined.hpp"
#include "symbol/function.hpp"
#include "ast/function/function_base.hpp"
#include "context.hpp"
#include "builtin.hpp"
#include "builtins.hpp"
#include "message_helper.hpp"
#include "assembly/create.hpp"

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

bool lang::ops::UserDefinedOperator::invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, optional_ref<ast::ConditionalContext> conditional) const {
  // ensure the function is defined
  if (!symbol().define(ctx))
    return false;

  // get function's location
  auto function_loc = ctx.symbols.locate(symbol().id());
  assert(function_loc.has_value());

  return ops::call_function(
      ctx.symbols.resolve_location(function_loc->get()),
      symbol().full_name(),
      type(),
      args,
      {},
      return_value,
      ctx
  );
}

bool lang::ops::BuiltinOperator::invoke(Context& ctx, const std::deque<std::unique_ptr<ast::Node>>& args, value::Value& return_value, optional_ref<ast::ConditionalContext> conditional) const {
  // generate & accumulate argument values
  std::deque<std::reference_wrapper<const value::Value>> arg_values;
  for (auto& arg : args) {
    if (!arg->generate_code(ctx)) return false;
    if (!arg->value().is_rvalue()) {
      ctx.messages.add(util::error_expected_lrvalue(*arg, arg->value().type(), false));
      return false;
    }
    arg_values.push_back(arg->value());
  }

  // generate code, attach rvalue reference
  uint8_t reg = generator_(ctx, arg_values);
  const auto ref = memory::Ref::reg(reg);
  return_value.rvalue(ref);

  // add comment
  auto& comment = ctx.program.current().back().comment();
  comment << "operator" << op() << "(";
  for (auto& value : arg_values) {
    value.get().type().print_code(comment);
    if (&value != &arg_values.back()) comment << ", ";
  }
  comment << ")";

  // "spoil" the current register
  auto& object = ctx.reg_alloc_manager.find(ref);
  object.value->rvalue(std::make_unique<value::RValue>(return_value.type(), ref));

  return true;
}

bool lang::ops::RelationalBuiltinOperator::invoke(Context& ctx, const std::deque<std::unique_ptr<ast::Node>>& args, value::Value& return_value, optional_ref<ast::ConditionalContext> conditional) const {
  // if no condition, proceed as normal
  if (!conditional.has_value()) {
    return BuiltinOperator::invoke(ctx, args, return_value, conditional);
  }

  // relational operators *must* be binary
  assert(args.size() == 2);

  // generate & accumulate children values
  std::deque<std::reference_wrapper<const value::Value>> arg_values;
  for (auto& arg : args) {
    if (!arg->generate_code(ctx)) return false;
    if (!arg->value().is_rvalue()) {
      ctx.messages.add(util::error_expected_lrvalue(*arg, arg->value().type(), false));
      return false;
    }
    arg_values.push_back(arg->value());
  }

  // generate comparison instruction
  const auto lhs_ref = ctx.reg_alloc_manager.guarantee_datatype(arg_values.front().get().rvalue().ref(), datatype_);
  const auto rhs_ref = ctx.reg_alloc_manager.guarantee_datatype(arg_values.back().get().rvalue().ref(), datatype_);
  ctx.program.current().add(assembly::create_comparison(
      datatype_.get_asm_datatype(),
      lhs_ref.offset,
      ctx.reg_alloc_manager.resolve_ref(rhs_ref, true)
    ));

  // add comment
  print_code(ctx.program.current().back().comment());

  // conditional jump(s) to true and false destination blocks
  conditional->get().generate_branches(ctx.program.current(), flag_);

  return true;
}

bool lang::ops::BooleanNotBuiltinOperator::invoke(Context& ctx, const std::deque<std::unique_ptr<ast::Node>>& args, value::Value& return_value, optional_ref<ast::ConditionalContext> conditional) const {
  // if no condition, proceed as normal
  if (!conditional.has_value()) {
    return BuiltinOperator::invoke(ctx, args, return_value, conditional);
  }

  // we must be unary
  assert(args.size() == 1);

  // propagate the inverse conditional to child
  ast::ConditionalContext inverse = conditional->get().inverse();
  args.front()->conditional_context(inverse);

  // generate child's code
  if (!args.front()->generate_code(ctx)) return false;

  // has this been handled?
  if (inverse.handled) {
    conditional->get().handled = true;
    return true;
  }

  // otherwise, we have a Boolean
  // do not call BuiltinOperator::invoke as this will re-generate argument's code

  // get the result (which will be a Boolean as '!' is defined on Booleans)
  auto& result_value = args.front()->value();
  if (!result_value.is_rvalue()) {
    ctx.messages.add(util::error_expected_lrvalue(*args.front(), result_value.type(), false));
    return false;
  }
  memory::Ref result = ctx.reg_alloc_manager.guarantee_register(result_value.rvalue().ref());

  // compare to zero
  ctx.program.current().add(assembly::create_comparison(
      result.offset,
      assembly::Arg::imm(0)
    ));

  // as we are the inverse, the default is to compare != 0
  conditional->get().generate_branches(ctx.program.current(), constants::cmp::ne);

  return true;
}
