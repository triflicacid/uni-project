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
#include "ast/types/bool.hpp"

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
      matches.push_back(std::cref(*op));
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

bool lang::ops::UserDefinedOperator::invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, const InvocationOptions& options) const {
  // ensure the function is defined
  if (!symbol().define(ctx))
    return false;

  // get function's location
  auto function_loc = ctx.symbols.locate(symbol().id());
  assert(function_loc.has_value());

  return ops::call_function(
      function_loc->get().resolve(true),
      symbol().full_name(),
      type(),
      args,
      {},
      return_value,
      {},
      ctx
  );
}

bool lang::ops::BuiltinOperator::invoke(Context& ctx, const std::deque<std::unique_ptr<ast::Node>>& args, value::Value& return_value, const InvocationOptions& options) const {
  // generate & accumulate argument values
  std::deque<std::reference_wrapper<const value::Value>> arg_values;
  for (int i = 0; i < args.size(); i++) {
    auto& arg = args[i];
    if (!arg->generate_code(ctx)) return false;

    // materialise value, check if rvalue
    auto& value = arg->value();
    value.materialise(ctx, arg->token_start().loc);
    if (!value.is_rvalue()) {
      ctx.messages.add(util::error_expected_lrvalue(*arg, value.type(), false));
      return false;
    }
    arg_values.push_back(value);
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

  // free arguments
  for (auto &arg : args) {
    // we know it is an rvalue
    ctx.reg_alloc_manager.mark_free(arg->value().rvalue().ref());
  }

  // reserve returned register
  // we know object exists, so directly manipulate it
  auto& object = ctx.reg_alloc_manager.find(ref);
  object.required = true;
  object.value->rvalue(std::make_unique<value::RValue>(return_value.type(), ref));

  return true;
}

bool lang::ops::RelationalBuiltinOperator::invoke(Context& ctx, const std::deque<std::unique_ptr<ast::Node>>& args, value::Value& return_value, const InvocationOptions& options) const {
  // if no condition, proceed as normal
  if (!options.conditional) {
    return BuiltinOperator::invoke(ctx, args, return_value, options);
  }

  // relational operators *must* be binary
  assert(args.size() == 2);

  // generate & accumulate children values
  std::deque<std::reference_wrapper<const value::Value>> arg_values;
  for (auto& arg : args) {
    if (!arg->generate_code(ctx)) return false;
    arg->value().materialise(ctx, arg->token_start().loc);
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
  options.conditional->get().generate_branches(ctx.program.current(), flag_);

  return true;
}

bool lang::ops::BooleanNotBuiltinOperator::invoke(Context& ctx, const std::deque<std::unique_ptr<ast::Node>>& args, value::Value& return_value, const InvocationOptions& options) const {
  // if no condition, proceed as normal
  if (!options.conditional) {
    return BuiltinOperator::invoke(ctx, args, return_value, options);
  }

  // we must be unary
  assert(args.size() == 1);
  auto& arg = *args.front();

  // propagate the inverse conditional to child
  control_flow::ConditionalContext inverse = options.conditional->get().inverse();
  arg.conditional_context(inverse);

  // generate child's code
  if (!arg.generate_code(ctx)) return false;
  arg.value().materialise(ctx, arg.token_start().loc);

  // was the inverse handled?
  options.conditional->get().handled = true;
  if (inverse.handled) {
    return true;
  }

  // otherwise, we have a Boolean
  // do not call BuiltinOperator::invoke as this will re-generate argument's code
  // don't update origins - this will be passed to the parent, as branching is up to them
  return inverse.generate_branches(ctx, *args.front(), args.front()->value());
}

static unsigned int lazy_op_current_id = 0;

lang::ops::LazyLogicalOperator::LazyLogicalOperator(std::string symbol, const lang::ast::type::FunctionNode& type, bool is_and)
    : BuiltinOperator(std::move(symbol), type, nullptr), and_(is_and) {}

bool lang::ops::LazyLogicalOperator::invoke(Context& ctx, const std::deque<std::unique_ptr<ast::Node>>& args, value::Value& return_value, const InvocationOptions& options) const {
  const unsigned int id = lazy_op_current_id++;

  // represent the RHS of the binary op
  auto rhs_block = assembly::BasicBlock::labelled("rhs_" + std::to_string(id));
  rhs_block->comment() << "rhs of " << op();

  if (options.conditional) {
    // handle LHS - do we need both, or just one?
    control_flow::ConditionalContext cond_ctx;
    if (and_) {
      cond_ctx.if_true = *rhs_block;
      cond_ctx.if_false = options.conditional->get().if_false;
    } else {
      cond_ctx.if_true = options.conditional->get().if_true;
      cond_ctx.if_false = *rhs_block;
    }

    auto& lhs = *args.front();
    lhs.conditional_context(cond_ctx);
    if (!lhs.generate_code(ctx)) return false;
    int index = ctx.program.current().size();
    if (!cond_ctx.handled && !cond_ctx.generate_branches(ctx, lhs, lhs.value())) return false;
    ctx.program.update_line_origins(lhs.token_start().loc, index);

    // handle RHS, use original conditional
    ctx.program.insert(assembly::Position::Next, std::move(rhs_block));
    auto& rhs = *args.back();
    rhs.conditional_context(options.conditional->get());
    if (!rhs.generate_code(ctx)) return false;
    index = ctx.program.current().size();
    if (!options.conditional->get().handled && !options.conditional->get().generate_branches(ctx, rhs, rhs.value())) return false;
    ctx.program.update_line_origins(rhs.token_start().loc, index);

    return true;
  }

  // blocks for `true` and `false` results, as well as an `after` block
  auto true_block = assembly::BasicBlock::labelled("true_" + std::to_string(id));
  auto false_block = assembly::BasicBlock::labelled("false_" + std::to_string(id));
  auto after_block = assembly::BasicBlock::labelled("end_" + std::to_string(id));

  // populate above blocks
  true_block->add(assembly::create_load(
      constants::registers::ret,
      assembly::Arg::imm(1)
    ));
  true_block->add(assembly::create_branch(assembly::Arg::label(*after_block)));

  false_block->add(assembly::create_zero(
      constants::registers::ret
  ));
  false_block->add(assembly::create_branch(assembly::Arg::label(*after_block)));

  // LHS portion
  control_flow::ConditionalContext cond_ctx;
  if (and_) {
    cond_ctx.if_false = *false_block;
    cond_ctx.if_true = *rhs_block;
  } else {
    cond_ctx.if_false = *rhs_block;
    cond_ctx.if_true = *true_block;
  }

  auto& lhs = *args.front();
  lhs.conditional_context(cond_ctx);
  if (!lhs.generate_code(ctx)) return false;
  lhs.value().materialise(ctx, lhs.token_start().loc);
  int index = ctx.program.current().size();
  if (!cond_ctx.handled && !cond_ctx.generate_branches(ctx, lhs, lhs.value())) return false;
  ctx.program.update_line_origins(lhs.token_end().loc, index);

  // RHS portion
  ctx.program.insert(assembly::Position::Next, std::move(rhs_block));
  cond_ctx = {
      .if_true = *true_block,
      .if_false = *false_block,
  };
  auto& rhs = *args.back();
  rhs.conditional_context(cond_ctx);
  if (!rhs.generate_code(ctx)) return false;
  rhs.value().materialise(ctx, rhs.token_start().loc);
  index = ctx.program.current().size();
  if (!cond_ctx.handled && !cond_ctx.generate_branches(ctx, rhs, rhs.value())) return false;
  ctx.program.update_line_origins(rhs.token_end().loc, index);

  // insert remaining blocks
  ctx.program.insert(assembly::Position::Next, std::move(true_block));
  ctx.program.update_line_origins(options.origin, 0);
  ctx.program.insert(assembly::Position::Next, std::move(false_block));
  ctx.program.update_line_origins(options.origin, 0);
  ctx.program.insert(assembly::Position::Next, std::move(after_block));
  ctx.program.update_line_origins(options.origin, 0);

  // set $ret to a Boolean
  ctx.reg_alloc_manager.update_ret(memory::Object(value::rvalue(
      ast::type::boolean,
      memory::Ref::reg(constants::registers::ret)
    )));

  // update return value
  return_value.rvalue(std::make_unique<value::RValue>(ast::type::boolean, memory::Ref::reg(constants::registers::ret)));

  return true;
}
