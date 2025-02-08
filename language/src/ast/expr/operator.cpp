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

std::string lang::ast::OperatorNode::node_name() const {
  return args_.size() == 2
    ? "binary operator"
    : "unary operator";
}

bool lang::ast::OperatorNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  for (auto& arg : args_) {
    if (!arg->collate_registry(messages, registry)) return false;
  }
  return true;
}

bool lang::ast::OperatorNode::process(lang::Context& ctx) {
  for (auto& arg : args_) {
    if (!arg->process(ctx)) return false;
  }
  return true;
}

std::ostream& lang::ast::OperatorNode::print_code(std::ostream& os, unsigned int indent_level) const {
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

std::ostream& lang::ast::OperatorNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << symbol() << SHELL_RESET;

  for (auto& arg : args_) {
    os << std::endl;
    arg->print_tree(os, indent_level + 1);
  }
  return os;
}

optional_ref<const lang::value::Value> lang::ast::OperatorNode::get_arg_rvalue(Context& ctx, int i) const {
  auto& arg = args_[i];
  const value::Value& value = arg->value();
  if (value.is_future_rvalue() && !arg->resolve_rvalue(ctx)) return {};
  // must be computable to a value (i.e., possible rvalue)
  if (!value.is_rvalue()) {
    auto msg = arg->generate_message(message::Error);
    msg->get() << "expected rvalue, got ";
    value.type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "while evaluating " << node_name();
    ctx.messages.add(std::move(msg));
    return {};
  }

  return value;
}

bool lang::ast::OverloadableOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;

  // generate our signature
  std::deque<std::reference_wrapper<const type::Node>> arg_types;
  for (auto& arg : args_) {
    // fetch argument's value
    const value::Value& value = arg->value();
    if (value.is_future_rvalue() && !arg->resolve_rvalue(ctx)) return false;
    // must be computable to a value (i.e., possible rvalue)
    if (!value.is_rvalue()) {
      auto msg = arg->generate_message(message::Error);
      msg->get() << "expected rvalue, got ";
      value.type().print_code(msg->get());
      ctx.messages.add(std::move(msg));

      msg = op_symbol_.generate_message(message::Note);
      msg->get() << "while evaluating " << node_name();
      ctx.messages.add(std::move(msg));
      return false;
    }
    arg_types.push_back(value.type());
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
    value_ = value::rvalue(op_->get().type().returns());
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
  std::unique_ptr<message::BasicMessage> msg = generate_message(message::Error);
  msg->get() << "unable to resolve a suitable candidate for " << to_string();
  ctx.messages.add(std::move(msg));

  for (auto& option : operators) {
    msg = std::make_unique<message::BasicMessage>(message::Note);
    msg->get() << "candidate: ";
    option.get().print_code(msg->get());
    ctx.messages.add(std::move(msg));
  }
  return false;
}

std::string lang::ast::OverloadableOperatorNode::to_string() const {
  std::stringstream s;
  s << "operator" << symbol();
  if (signature_.has_value()) {
    signature_->get().print_code(s);
  } else {
    s << "(?)";
  }
  return s.str();
}

bool lang::ast::OverloadableOperatorNode::resolve_rvalue(lang::Context& ctx) {
  // operator must have been set by ::process
  assert(op_.has_value());

  for (int i = 0; i < args_.size(); i++) {
    if (!get_arg_rvalue(ctx, i)) return false;
  }

  // TODO user-defined operators
  auto& op = op_.value().get();
  assert(op.builtin());

  // accumulate argument values
  auto& builtin = static_cast<const ops::BuiltinOperator&>(op);
  std::deque<std::reference_wrapper<const value::Value>> args;
  for (auto& arg : args_) args.push_back(arg->value());
  // generate code, attach rvalue reference
  uint8_t reg = builtin.generator()(ctx, args);
  value_->rvalue(memory::Ref::reg(reg));
  ctx.program.current().back().comment() << to_string();

  return true;
}

std::unique_ptr<lang::ast::OperatorNode>
lang::ast::OperatorNode::unary(lexer::Token token, std::unique_ptr<Node> expr) {
  std::deque<std::unique_ptr<Node>> children;
  children.push_back(std::move(expr));
  return std::make_unique<OverloadableOperatorNode>(token, token, std::move(children));
}

std::unique_ptr<lang::ast::OperatorNode>
lang::ast::OperatorNode::binary(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs) {
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

lang::ast::Node& lang::ast::OperatorNode::lhs_() const {
  assert(args_.size() > 0);
  return *args_.front();
}

lang::ast::Node& lang::ast::OperatorNode::rhs_() const {
  assert(args_.size() > 1);
  return *args_[1];
}

lang::ast::CStyleCastOperatorNode::CStyleCastOperatorNode(lang::lexer::Token token, const lang::ast::type::Node& target, std::unique_ptr<Node> expr)
  : OperatorNode(token, token, {}), target_(target) {
  args_.push_back(std::move(expr));

  std::stringstream stream;
  stream << '(';
  target_.print_code(stream);
  stream << ')';
  symbol_ = stream.str();

  op_symbol_.image = symbol_; // doctor image for correct-looking error reporting
}

bool lang::ast::CStyleCastOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;
  value_ = value::rvalue(target_);
  return true;
}

bool lang::ast::CStyleCastOperatorNode::resolve_rvalue(lang::Context& ctx) {
  if (!get_arg_rvalue(ctx, 0)) return false;

  // emit conversion instruction
  const memory::Ref ref = ops::cast(ctx, target_);
  value_->rvalue(std::make_unique<value::RValue>(target_, ref));
  // TODO more checks for Boolean
  return true;
}

lang::ast::DotOperatorNode::DotOperatorNode(lang::lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
  : OperatorNode(std::move(token), std::move(symbol), {}) {
  token_end(rhs->token_end());
  args_.push_back(std::move(lhs));
  args_.push_back(std::move(rhs));
}

bool lang::ast::DotOperatorNode::process(lang::Context& ctx) {
  // get rvalue LHS
  if (!lhs_().process(ctx)) return false;
  const auto& lhs = lhs_().value();
  if (lhs.is_future_rvalue() && !lhs_().resolve_rvalue(ctx)) return false;
  // must be an lvalue
  if (!lhs.is_lvalue()) {
    auto msg = lhs_().generate_message(message::Error);
    msg->get() << "expected lvalue, got ";
    lhs.type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "while evaluating " << node_name();
    ctx.messages.add(std::move(msg));
    return false;
  }

  // get the RHS
  if (!rhs_().process(ctx)) return false;
  const auto& rhs = rhs_().value();
  // expect a SymbolRef, anything else...
  if (auto ref = rhs.get_symbol_ref()) {
    attr_ = ref->get();
  } else {
    auto msg = rhs_().generate_message(message::Error);
    msg->get() << "expected symbol, got ";
    rhs.type().print_code(msg->get());
    msg->get() << " value";
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "on rhs of dot operator";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // if base is a namespace, then this is a symbol reference
  // TODO add support for structs here
  if (lhs.type() == type::name_space) {
    const symbol::Symbol& symbol = lhs.lvalue().get_symbol()->get();
    const std::string full_name = symbol.full_name() + "." + attr_;

    // for good error reporting, check that the name at least exists
    if (ctx.symbols.find(full_name).empty()) {
      ctx.messages.add(util::error_no_member(rhs_(), symbol.type(), symbol.full_name(), attr_));
      return false;
    }

    value_ = value::symbol_ref(full_name);
  }

  throw std::runtime_error("DotOperatorNode::get_value");
}

bool lang::ast::DotOperatorNode::resolve_lvalue(lang::Context& ctx) {
  // attempt to resolve if SymbolRef
  if (auto ref = value().get_symbol_ref()) {
    auto symbol = ref->resolve(ctx, *this, true);
    if (!symbol) return false;
    value_->lvalue(std::move(symbol));
    return true;
  }

  throw std::runtime_error("DotOperatorNode::resolve_lvalue");
}

bool lang::ast::DotOperatorNode::resolve_rvalue(lang::Context& ctx) {
  if (!value().is_lvalue() && !resolve_lvalue(ctx)) return false;

  // get attached symbol
  const value::Symbol* symbol = value().lvalue().get_symbol();
  assert(symbol != nullptr);

  // load into registers
  if (symbol->type().size() == 0) return true;
  const memory::Ref ref = ctx.reg_alloc_manager.find(symbol->get());
  value_->rvalue(std::make_unique<value::RValue>(symbol->type(), ref));
  return true;
}

lang::ast::AssignmentOperatorNode::AssignmentOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
    : OperatorNode(std::move(token), std::move(symbol), {}) {
  token_end(rhs->token_end());
  args_.push_back(std::move(lhs));
  args_.push_back(std::move(rhs));
}

bool lang::ast::AssignmentOperatorNode::process(lang::Context& ctx) {
  // process both arguments
  if (!lhs_().process(ctx) || !rhs_().process(ctx)) return false;

  // lhs must be an lvalue
  auto& lhs = lhs_().value();
  if (lhs.is_future_rvalue() && !lhs_().resolve_rvalue(ctx)) return false;
  if (!lhs.is_lvalue()) {
    auto msg = lhs_().generate_message(message::Error);
    msg->get() << "expected lvalue, got ";
    lhs.type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "while evaluating " << node_name();
    ctx.messages.add(std::move(msg));
    return false;
  }

  return true;
}

bool lang::ast::AssignmentOperatorNode::resolve_lvalue(lang::Context& ctx) {
  // ::get_value should return an rvalue anyway
  assert(value().is_lvalue());
  return true;
}

bool lang::ast::AssignmentOperatorNode::resolve_rvalue(lang::Context& ctx) {
  const auto& lhs = value();
  assert(lhs.is_lvalue());

  // if symbol is constant, error
  if (auto symbol_value = lhs.lvalue().get_symbol()) {
    const symbol::Symbol& symbol = symbol_value->get();
    if (symbol.is_constant()) {
      auto msg = op_symbol_.generate_message(message::Error);
      msg->get() << "unable to assign to constant symbol";
      ctx.messages.add(std::move(msg));

      msg = symbol.token().generate_message(message::Note);
      msg->get() << "symbol " << symbol.full_name() << " defined here";
      ctx.messages.add(std::move(msg));
      return false;
    }
  }

  // evaluate RHS
  auto& rhs = rhs_().value();
  if (!rhs_().resolve_rvalue(ctx)) return false;

  // check that types match
  if (!type::graph.is_subtype(rhs.type().id(), lhs.type().id())) {
    ctx.messages.add(util::error_type_mismatch(op_symbol_, rhs.type(), lhs.type(), true));
    return false;
  }

  // coerce into correct type (this is safe as subtyping checked)
  const memory::Ref& expr = ctx.reg_alloc_manager.guarantee_datatype(rhs.rvalue().ref(), lhs.type());
  value_->rvalue(expr);

  // load expr value into symbol
  // TODO what if LHS if not a symbol
  auto symbol = lhs.lvalue().get_symbol();
  assert(symbol != nullptr);
  ctx.symbols.assign_symbol(symbol->get().id(), expr.offset);

  // if symbol, evict it as it has been updated
  ctx.reg_alloc_manager.evict(symbol->get());

  return true;
}

lang::ast::CastOperatorNode::CastOperatorNode(lang::lexer::Token token, const lang::ast::type::Node& target, std::unique_ptr<Node> expr)
: OperatorNode(token, token, {}), target_(target) {
  args_.push_back(std::move(expr));
}

bool lang::ast::CastOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;
  value_ = value::rvalue(target_);
  return true;
}

bool lang::ast::CastOperatorNode::resolve_rvalue(lang::Context& ctx) {
  if (!get_arg_rvalue(ctx, 0)) return false;

  // emit conversion instruction
  const memory::Ref ref = ops::cast(ctx, target_);
  value_->rvalue(std::make_unique<value::RValue>(target_, ref));
  return true;
}

std::ostream& lang::ast::CastOperatorNode::print_code(std::ostream& os, unsigned int indent_level) const {
  args_.front()->print_code(os) << " as ";
  return target_.print_code(os);
}

std::ostream& lang::ast::CastOperatorNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << "as" << SHELL_RESET " " SHELL_CYAN;
  target_.print_code(os) << SHELL_RESET << std::endl;

  args_.front()->print_tree(os, indent_level + 1);
  return os;
}
