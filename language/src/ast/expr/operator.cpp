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

bool lang::ast::expr::OverloadableOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;

  // generate our signature
  std::deque<std::reference_wrapper<const type::Node>> arg_types;
  std::deque<std::unique_ptr<value::Value>> values;
  for (auto& arg : args_) {
    // fetch argument's value
    auto value = arg->get_value(ctx);
    if (!value) return false;
    // if symbol reference, attempt to resolve
    if (auto ref = value->get_symbol_ref()) {
      value = ref->resolve(ctx, arg->token_start(), true);
      if (!value) return false;
    }
    // must be computable to a value (i.e., possible rvalue)
    if (!value->is_computable()) {
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
  return std::make_unique<value::Temporary>(op_->get().type().returns(), value::Options{.computable=true});
}

std::unique_ptr<lang::value::Value> lang::ast::expr::OverloadableOperatorNode::load(lang::Context& ctx) const {
  if (!load_and_check_rvalue(ctx)) return nullptr;

  // operator must have been set by ::process
  assert(op_.has_value());

  // TODO user-defined operators
  auto& op = op_.value().get();
  assert(op.builtin());

  // generate asm code and mark with comment
  auto& builtin = static_cast<const ops::BuiltinOperator&>(op);
  auto value = builtin.process(ctx);
  ctx.program.current().back().comment() << to_string();

  return value;
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

bool lang::ast::expr::OperatorNode::load_and_check_rvalue(Context& ctx) const {
  for (auto& arg : args_) {
    auto result = arg->load(ctx);
    if (!result) return false;

    // MUST be an rvalue or bad
    if (!result->is_rvalue()) {
      auto msg = arg->generate_message(message::Error);
      msg->get() << "expected rvalue, got ";
      result->type().print_code(msg->get());
      ctx.messages.add(std::move(msg));

      msg = op_symbol_.generate_message(message::Note);
      msg->get() << "while evaluating " << node_name();
      ctx.messages.add(std::move(msg));
      return false;
    }
  }
  return true;
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
  return std::make_unique<value::Temporary>(target_, value::Options{.computable=true});
}

std::unique_ptr<lang::value::Value> lang::ast::expr::CastOperatorNode::load(lang::Context& ctx) const {
  if (!load_and_check_rvalue(ctx)) return nullptr;

  // emit conversion instruction
  auto value = get_value(ctx);
  const memory::Ref ref = ops::implicit_cast(ctx, target_.get_asm_datatype());
  value->set_ref(ref);
  return value;
}

lang::ast::expr::DotOperatorNode::DotOperatorNode(lang::lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
  : OperatorNode(std::move(token), std::move(symbol), {}) {
  token_end(rhs->token_end());
  args_.push_back(std::move(lhs));
  args_.push_back(std::move(rhs));
}

bool lang::ast::expr::DotOperatorNode::process(lang::Context& ctx) {
  if (!lhs_().process(ctx)) return false;

  // get LHS (do not evaluate though)
  const auto lhs = lhs_().get_value(ctx);
  if (!lhs) return false;

  // fetch symbol referenced by LHS
  const symbol::Symbol* parent = nullptr;
  if (auto symbol = lhs->get_symbol()) {
    parent = &symbol->get();
  } else if (auto ref = lhs->get_symbol_ref()) {
    auto symbols = ctx.symbols.find(ref->get());
    if (symbols.empty()) {
      ctx.messages.add(util::error_unresolved_symbol(lhs_(), ref->get()));
      return false;
    } else if (symbols.size() > 1) {
      ctx.messages.add(util::error_insufficient_info_to_resolve_symbol(lhs_(), ref->get()));
      return false;
    }
    parent = &symbols.front().get();
  } else {
    auto msg = lhs_().generate_message(message::Error);
    msg->get() << "expected symbol, got ";
    lhs->type().print_code(msg->get());
    msg->get() << " value";
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "on lhs of dot operator";
    ctx.messages.add(std::move(msg));
    return false;
  }

  const std::string name_lhs = parent->full_name();

  // get the RHS
  const auto rhs = rhs_().get_value(ctx);
  if (!rhs) return false;

  // unlike LHS, this symbol may not be fully resolved
  std::string name_rhs;
  if (auto symbol = rhs->get_symbol()) {
    name_rhs = symbol->get().name();
  } else if (auto ref = rhs->get_symbol_ref()) {
    name_rhs = ref->get();
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

  // this is the name we refer to:
  resolved_ = name_lhs + "." + name_rhs;

  // lookup symbol, ensure that we exist
  auto symbols = ctx.symbols.find(resolved_);
  if (symbols.empty()) {
    ctx.messages.add(util::error_no_member(rhs_(), parent->type(), name_lhs, name_rhs));
    resolved_.clear(); // clear to prevent invalid state going forwards
    return false;
  }

  // if only one hit, that's our guy
  if (symbols.size() == 1) {
    symbol_ = symbols.front();
    return true;
  }

  return true;
}

std::unique_ptr<lang::value::Value> lang::ast::expr::DotOperatorNode::get_value(lang::Context& ctx) const {
  assert(!resolved_.empty());

  // if symbol is recorded, return reference to it
  if (symbol_.has_value()) {
    auto location = ctx.symbols.locate(symbol_->get().id());
    return std::make_unique<value::Symbol>(*symbol_, value::Options{.lvalue=location, .computable=location.has_value()});
  }

  // otherwise, just return reference to name
  // we can figure it out elsewhere :)
  return std::make_unique<value::SymbolRef>(resolved_);
}

std::unique_ptr<lang::value::Value> lang::ast::expr::DotOperatorNode::load(lang::Context& ctx) const {
  auto value = get_value(ctx);
  if (!value) return nullptr;

  // if SymbolRef, we do not have sufficient information to continue
  if (auto ref = value->get_symbol_ref()) {
    ctx.messages.add(util::error_unresolved_symbol(*this, ref->get()));

    auto symbols = ctx.symbols.find(ref->get());
    for (auto& symbol : symbols) {
      auto msg = symbol.get().token().generate_message(message::Note);
      msg->get() << "candidate: " << symbol.get().full_name();
      ctx.messages.add(std::move(msg));
    }
    return nullptr;
  }

  // note, must be a symbol here
  auto symbol = value->get_symbol();
  assert(symbol);

  // must be computable, otherwise has no size
  if (!symbol->is_computable()) {
//    auto msg = rhs_().token().generate_message(message::Error);
//    msg->get() << "expected rvalue, got ";
//    symbol->type().print_code(msg->get());
//    ctx.messages.add(std::move(msg));
//    return nullptr;
    return value;
  }

  // load into register if symbol
  memory::Ref ref = ctx.reg_alloc_manager.find(static_cast<const symbol::Variable&>(symbol->get()));
  value->set_ref(ref);
  return value;
}
