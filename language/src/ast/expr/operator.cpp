#include <cassert>
#include "operator.hpp"
#include "shell.hpp"
#include "ast/types/function.hpp"
#include "operators/operator.hpp"
#include "context.hpp"
#include "operators/builtin.hpp"
#include "ast/types/namespace.hpp"
#include "ast/expr/symbol_reference.hpp"
#include "ast/namespace.hpp"
#include "message_helper.hpp"
#include "value/symbol.hpp"
#include "ast/types/graph.hpp"

std::string lang::ast::expr::OperatorNode::node_name() const {
  return args_.size() == 2
    ? "binary operator"
    : "unary operator";
}

bool lang::ast::expr::OperatorNode::process(lang::Context& ctx) {
  for (auto& arg : args_) {
    if (!arg->process(ctx)) return false;
  }
  return true;
}

std::ostream& lang::ast::expr::OperatorNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "(";
  if (args_.size() == 1) {
    os << symbol();
    args_.front()->print_code(os, indent_level);
  } else if (args_.size() == 2) {
    args_.front()->print_code(os, indent_level);
    os << " " << symbol() << " ";
    args_.back()->print_code(os, indent_level);
  }
  return os << ")";
}

std::ostream& lang::ast::expr::OperatorNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << symbol() << SHELL_RESET;

  for (auto& arg : args_) {
    os << std::endl;
    arg->print_tree(os, indent_level + 1);
  }
  return os;
}

std::unique_ptr<lang::value::Value> lang::ast::expr::OperatorNode::get_arg_rvalue(Context& ctx, int i) const {
  const auto& arg = args_[i];
  auto value = arg->get_value(ctx);
  if (value->is_future_rvalue() && !arg->resolve_rvalue(ctx, *value)) return nullptr;
  // must be computable to a value (i.e., possible rvalue)
  if (!value->is_rvalue()) {
    auto msg = arg->generate_message(message::Error);
    msg->get() << "expected rvalue, got ";
    value->type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "while evaluating " << node_name();
    ctx.messages.add(std::move(msg));
    return nullptr;
  }

  return value;
}

bool lang::ast::expr::OverloadableOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;

  // generate our signature
  std::deque<std::reference_wrapper<const type::Node>> arg_types;
  std::deque<std::unique_ptr<value::Value>> values;
  for (auto& arg : args_) {
    // fetch argument's value
    auto value = arg->get_value(ctx);
    if (value->is_future_rvalue() && !arg->resolve_rvalue(ctx, *value)) return false;
    // must be computable to a value (i.e., possible rvalue)
    if (!value->is_rvalue()) {
      auto msg = arg->generate_message(message::Error);
      msg->get() << "expected rvalue, got ";
      value->type().print_code(msg->get());
      ctx.messages.add(std::move(msg));

      msg = op_symbol_.generate_message(message::Note);
      msg->get() << "while evaluating " << node_name();
      ctx.messages.add(std::move(msg));
      return false;
    }
    arg_types.push_back(value->type());
    values.push_back(std::move(value));
  }
  const auto& signature = type::FunctionNode::create(arg_types, std::nullopt);
  signature_ = signature;

  // search for a matching operator
  auto operators = ops::get(symbol());

  // extract types into an option list
  std::deque<std::reference_wrapper<const type::FunctionNode>> candidates;
  for (auto& op : operators) {
    candidates.push_back(op.get().type());
  }

  // filter to viable candidates
  candidates = signature.filter_candidates(candidates);

  // if only one candidate remaining: this is us! use signature + name to find operator object
  if (candidates.size() == 1) {
    op_ = ops::get(symbol(), candidates.front());
    signature_ = op_->get().type(); // make sure to replace with the correct signature (esp. for the return type)
    return true;
  }

  // otherwise, if candidates is non-empty, we can narrow options down for message
  if (!candidates.empty()) {
    operators.clear();
    for (auto& candidate : candidates) {
      operators.push_back(ops::get(symbol(), candidate).value());
    }
  }

  // error to user, reporting our candidate list
  std::unique_ptr<message::BasicMessage> msg = op_symbol_.generate_message(message::Error);
  msg->get() << "unable to resolve a suitable candidate for " << to_string();
  ctx.messages.add(std::move(msg));

  for (auto& option : operators) {
    msg = std::make_unique<message::BasicMessage>(message::Note);
    msg->get() << "candidate: " << to_string();
    ctx.messages.add(std::move(msg));
  }
  return false;
}

std::string lang::ast::expr::OverloadableOperatorNode::to_string() const {
  std::stringstream s;
  s << "operator" << symbol();
  if (signature_.has_value()) {
    signature_->get().print_code(s);
  } else {
    s << "(?)";
  }
  return s.str();
}

std::unique_ptr<lang::value::Value> lang::ast::expr::OverloadableOperatorNode::get_value(lang::Context& ctx) const {
  assert(op_.has_value());
  return value::rvalue(op_->get().type().returns());
}

bool lang::ast::expr::OverloadableOperatorNode::resolve_rvalue(lang::Context& ctx, lang::value::Value& value) const {
  for (int i = 0; i < args_.size(); i++) {
    if (!get_arg_rvalue(ctx, i)) return false;
  }

  // operator must have been set by ::process
  assert(op_.has_value());

  // TODO user-defined operators
  auto& op = op_.value().get();
  assert(op.builtin());

  // generate asm code and mark with comment
  auto& builtin = static_cast<const ops::BuiltinOperator&>(op);
  builtin.process(ctx, value);
  ctx.program.current().back().comment() << to_string();

  return true;
}

std::unique_ptr<lang::ast::expr::OperatorNode>
lang::ast::expr::OperatorNode::unary(lexer::Token token, std::unique_ptr<Node> expr) {
  std::deque<std::unique_ptr<Node>> children;
  children.push_back(std::move(expr));
  return std::make_unique<OverloadableOperatorNode>(token, token, std::move(children));
}

std::unique_ptr<lang::ast::expr::OperatorNode>
lang::ast::expr::OperatorNode::binary(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs) {
  // check for *special* operators
  if (symbol.image == ".") {
    return std::make_unique<DotOperatorNode>(std::move(token), std::move(symbol), std::move(lhs), std::move(rhs));
  } else if (symbol.image == "=") {
    return std::make_unique<AssignmentOperatorNode>(std::move(token), std::move(symbol), std::move(lhs), std::move(rhs));
  }

  // otherwise, generic binary op
  std::deque<std::unique_ptr<Node>> children;
  children.push_back(std::move(lhs));
  children.push_back(std::move(rhs));
  return std::make_unique<OverloadableOperatorNode>(std::move(token), std::move(symbol), std::move(children));
}

lang::ast::expr::Node& lang::ast::expr::OperatorNode::lhs_() const {
  assert(args_.size() > 0);
  return *args_.front();
}

lang::ast::expr::Node& lang::ast::expr::OperatorNode::rhs_() const {
  assert(args_.size() > 1);
  return *args_[1];
}

lang::ast::expr::CastOperatorNode::CastOperatorNode(lang::lexer::Token token, const lang::ast::type::Node& target, std::unique_ptr<Node> expr)
  : OperatorNode(token, token, {}), target_(target) {
  args_.push_back(std::move(expr));

  std::stringstream stream;
  stream << '(';
  target_.print_code(stream);
  stream << ')';
  symbol_ = stream.str();

  op_symbol_.image = symbol_; // doctor image for correct-looking error reporting
}

std::unique_ptr<lang::value::Value> lang::ast::expr::CastOperatorNode::get_value(lang::Context& ctx) const {
    return value::rvalue(target_);
}

bool lang::ast::expr::CastOperatorNode::resolve_rvalue(lang::Context& ctx, lang::value::Value& value) const {
  if (!get_arg_rvalue(ctx, 0)) return false;

  // emit conversion instruction
  const memory::Ref ref = ops::implicit_cast(ctx, target_.get_asm_datatype());
  value.rvalue(std::make_unique<value::RValue>(target_, ref));
  return true;
}

lang::ast::expr::DotOperatorNode::DotOperatorNode(lang::lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
  : OperatorNode(std::move(token), std::move(symbol), {}) {
  token_end(rhs->token_end());
  args_.push_back(std::move(lhs));
  args_.push_back(std::move(rhs));
}

bool lang::ast::expr::DotOperatorNode::process(lang::Context& ctx) {
  // get rvalue LHS
  if (!lhs_().process(ctx)) return false;
  base_ = lhs_().get_value(ctx);
  if (base_->is_future_rvalue() && !lhs_().resolve_rvalue(ctx, *base_)) return false;
  // must be an lvalue
  if (!base_->is_lvalue()) {
    auto msg = lhs_().generate_message(message::Error);
    msg->get() << "expected lvalue, got ";
    base_->type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "while evaluating " << node_name();
    ctx.messages.add(std::move(msg));
    return false;
  }

  // get the RHS
  if (!rhs_().process(ctx)) return false;
  const auto rhs = rhs_().get_value(ctx);
  if (!rhs) return false;

  // expect a SymbolRef, anything else...
  if (auto ref = rhs->get_symbol_ref()) {
    attr_ = ref->get();
  } else {
    auto msg = rhs_().generate_message(message::Error);
    msg->get() << "expected symbol, got ";
    rhs->type().print_code(msg->get());
    msg->get() << " value";
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "on rhs of dot operator";
    ctx.messages.add(std::move(msg));
    return false;
  }

  return true;
}

std::unique_ptr<lang::value::Value> lang::ast::expr::DotOperatorNode::get_value(lang::Context& ctx) const {
  assert(base_ && !attr_.empty());

  // if base is a namespace, then this is a symbol reference
  // TODO add support for structs here
  if (base_->type().id() == type::name_space.id()) {
    const symbol::Symbol& symbol = base_->lvalue().get_symbol()->get();
    const std::string full_name = symbol.full_name() + "." + attr_;

    // for good error reporting, check that the name at least exists
    if (ctx.symbols.find(full_name).empty()) {
      ctx.messages.add(util::error_no_member(rhs_(), symbol.type(), symbol.full_name(), attr_));
      return nullptr;
    }

    return std::make_unique<value::SymbolRef>(full_name);
  }

  throw std::runtime_error("DotOperatorNode::get_value");

  // otherwise, return generic lr-value
  return value::rlvalue();
}

bool lang::ast::expr::DotOperatorNode::resolve_lvalue(lang::Context& ctx, lang::value::Value& value) const {
  // attempt to resolve if SymbolRef
  if (auto ref = value.get_symbol_ref()) {
    auto symbol = ref->resolve(ctx, *this, true);
    if (!symbol) return false;
    value.lvalue(std::move(symbol));
    return true;
  }

  throw std::runtime_error("DotOperatorNode::resolve_lvalue");
  return false;
}

bool lang::ast::expr::DotOperatorNode::resolve_rvalue(lang::Context& ctx, lang::value::Value& value) const {
  if (!value.is_lvalue() && !resolve_lvalue(ctx, value)) return false;

  // get attached symbol
  const value::Symbol* symbol = value.lvalue().get_symbol();
  assert(symbol != nullptr);

  // load into registers
  if (symbol->type().size() == 0) return true;
  const memory::Ref ref = ctx.reg_alloc_manager.find(static_cast<const symbol::Variable&>(symbol->get()));
  value.rvalue(std::make_unique<value::RValue>(symbol->type(), ref));
  return true;
}

lang::ast::expr::AssignmentOperatorNode::AssignmentOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
    : OperatorNode(std::move(token), std::move(symbol), {}) {
  token_end(rhs->token_end());
  args_.push_back(std::move(lhs));
  args_.push_back(std::move(rhs));
}

bool lang::ast::expr::AssignmentOperatorNode::process(lang::Context& ctx) {
  // prepare LHS
  if (!lhs_().process(ctx)) return false;

  // prepare RHS
  if (!rhs_().process(ctx)) return false;

  return true;
}

std::unique_ptr<lang::value::Value> lang::ast::expr::AssignmentOperatorNode::get_value(lang::Context& ctx) const {
  auto value = lhs_().get_value(ctx);
  if (value->is_future_rvalue() && !lhs_().resolve_rvalue(ctx, *value)) return nullptr;
  // must be an lvalue
  if (!value->is_lvalue()) {
    auto msg = lhs_().generate_message(message::Error);
    msg->get() << "expected lvalue, got ";
    value->type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "while evaluating " << node_name();
    ctx.messages.add(std::move(msg));
    return nullptr;
  }

  return value;
}

bool lang::ast::expr::AssignmentOperatorNode::resolve_lvalue(lang::Context& ctx, lang::value::Value& value) const {
  // ::get_value should return an rvalue anyway
  assert(value.is_lvalue());
  return true;
}

bool lang::ast::expr::AssignmentOperatorNode::resolve_rvalue(lang::Context& ctx, lang::value::Value& lhs) const {
  assert(lhs.is_lvalue());

  // evaluate RHS
  auto rhs = rhs_().get_value(ctx);
  if (!rhs || !rhs_().resolve_rvalue(ctx, *rhs)) return false;

  // check that types match
  if (!type::graph.is_subtype(rhs->type().id(), lhs.type().id())) {
    ctx.messages.add(util::error_type_mismatch(op_symbol_, rhs->type(), lhs.type(), true));
    return false;
  }

  // coerce into correct type (this is safe as subtyping checked)
  const memory::Ref& expr = ctx.reg_alloc_manager.guarantee_datatype(rhs->rvalue().ref(), lhs.type().get_asm_datatype());
  lhs.rvalue(expr);

  // load expr value into symbol
  // TODO what if LHS if not a symbol
  auto symbol = lhs.lvalue().get_symbol();
  assert(symbol != nullptr);
  ctx.symbols.assign_symbol(symbol->get().id(), expr.offset);

  return true;
}

