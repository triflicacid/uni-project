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
#include "ast/types/int.hpp"
#include "ast/types/pointer.hpp"
#include "assembly/create.hpp"
#include "ast/function.hpp"
#include "operators/user_defined.hpp"

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

optional_ref<const lang::value::Value> lang::ast::OperatorNode::get_arg_lvalue(Context& ctx, int i) const {
  auto& arg = args_[i];
  const value::Value& value = arg->value();
  if (value.is_future_lvalue() && !arg->resolve_lvalue(ctx)) return {};
  // must be computable to a value (i.e., possible rvalue)
  if (!value.is_lvalue()) {
    auto msg = arg->generate_message(message::Error);
    msg->get() << "expected lvalue, got ";
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
    if (!arg->resolve_lvalue(ctx)) return false;
    arg_types.push_back(arg->value().type());
  }
  const auto& signature = type::FunctionNode::create(arg_types, std::nullopt);
  signature_ = signature;

  // try to find candidate
  auto maybe_op = ops::select_candidate(symbol(), signature, *this, ctx.messages);
  if (!maybe_op.has_value()) return false;
  op_ = maybe_op->get();

  // make sure to replace with the correct signature (esp. for the return type)
  signature_ = op_->get().type();

  // update rvalue return
  value_ = value::rvalue(op_->get().type().returns());
  return true;
}

std::string lang::ast::OverloadableOperatorNode::to_string() const {
  std::stringstream s;
  s << "operator" << symbol();
  if (signature_.has_value()) {
    signature_->get().print_code(s, false);
  } else {
    s << "(?)";
  }
  return s.str();
}

bool lang::ast::OverloadableOperatorNode::resolve_rvalue(lang::Context& ctx) {
  // operator must have been set by ::process
  assert(op_.has_value());

  // get the operator we resolved to
  auto& op = op_.value().get();

  // if not built-in, make a function call
  if (!op.builtin()) {
    auto& user_op = static_cast<const ops::UserDefinedOperator&>(op);
    auto function_loc = ctx.symbols.locate(user_op.symbol_id());
    assert(function_loc.has_value());

    ops::call_function(
        *function_loc,
        symbol(),
        *signature_,
        args_,
        *value_,
        ctx
      );
    return true;
  }

  // otherwise, we have a built-in
  auto& builtin = static_cast<const ops::BuiltinOperator&>(op);

  // evaluate & accumulate argument values
  std::deque<std::reference_wrapper<const value::Value>> args;
  for (int i = 0; i < args_.size(); i++) {
    if (auto value = get_arg_rvalue(ctx, i)) {
      args.push_back(value.value());
    } else {
      return false;
    }
  }

  // generate code, attach rvalue reference
  uint8_t reg = builtin.generator()(ctx, args);
  value_->rvalue(memory::Ref::reg(reg));
  ctx.program.current().back().comment() << to_string();

  return true;
}

std::unique_ptr<lang::ast::OperatorNode>
lang::ast::OperatorNode::unary(lexer::Token token, std::unique_ptr<Node> expr) {
  // check for *special* operators
  if (token.type == lexer::TokenType::registerof_kw) {
    return std::make_unique<RegisterOfOperatorNode>(token, token, std::move(expr));
  } else if (token.image == "&") {
    return std::make_unique<AddressOfOperatorNode>(token, token, std::move(expr));
  } else if (token.image == "*") {
    return std::make_unique<DereferenceOperatorNode>(token, token, std::move(expr));
  }

  // otherwise, generic unary op
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
  token_end(args_.back()->token_end());

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

  // fetch reference to arg and convert
  memory::Ref ref = args_.front()->value().rvalue().ref();
  ref = ctx.reg_alloc_manager.guarantee_datatype(ref, target_);

  // update rvalue
  value_->rvalue(std::make_unique<value::RValue>(target_, ref));
  return true;
}

lang::ast::DotOperatorNode::DotOperatorNode(lang::lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
  : OperatorNode(std::move(token), std::move(symbol), {}) {
  token_start(lhs->token_start());
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
  assert(lhs.type() == type::name_space);
  if (lhs.type() == type::name_space) {
    const symbol::Symbol& symbol = lhs.lvalue().get_symbol()->get();
    const std::string full_name = symbol.full_name() + "." + attr_;

    // for good error reporting, check that the name at least exists
    if (ctx.symbols.find(full_name).empty()) {
      ctx.messages.add(util::error_no_member(rhs_(), symbol.type(), symbol.full_name(), attr_));
      return false;
    }

    value_ = value::symbol_ref(full_name);
    return true;
  }

  // unreachable
  return false;
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
  const memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(symbol->get());
  value_->rvalue(std::make_unique<value::RValue>(symbol->type(), ref));
  return true;
}

lang::ast::AssignmentOperatorNode::AssignmentOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
    : OperatorNode(std::move(token), std::move(symbol), {}) {
  token_start(lhs->token_start());
  token_end(rhs->token_end());

  args_.push_back(std::move(lhs));
  args_.push_back(std::move(rhs));
}

bool lang::ast::AssignmentOperatorNode::process(lang::Context& ctx) {
  // process both arguments
  if (!lhs_().process(ctx) || !rhs_().process(ctx)) return false;

  // lhs must be an lvalue
  auto& lhs = lhs_().value();
  if (lhs.is_future_lvalue() && !lhs_().resolve_lvalue(ctx)) return false;
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

  // set value to LHS
  value_ = value::rlvalue(lhs.type());

  // check if LHS is a symbol
  if (auto symbol_value = lhs.lvalue().get_symbol()) {
    const symbol::Symbol& symbol = symbol_value->get();

    // cannot assign to a constant outside of declaration
    if (symbol.is_constant()) {
      auto msg = op_symbol_.generate_message(message::Error);
      msg->get() << "unable to assign to constant symbol";
      ctx.messages.add(std::move(msg));

      msg = symbol.token().generate_message(message::Note);
      msg->get() << "symbol " << symbol.full_name() << " defined here";
      ctx.messages.add(std::move(msg));
      return false;
    }

    // update resulting value
    value_->lvalue(std::make_unique<value::Symbol>(symbol));
  }

  return true;
}

bool lang::ast::AssignmentOperatorNode::resolve_rvalue(lang::Context& ctx) {
  const auto& lhs = lhs_().value();

  // evaluate RHS
  auto& rhs = rhs_().value();
  if (!rhs_().resolve_rvalue(ctx)) return false;

  // check that types match
  if (!type::graph.is_subtype(rhs.type().id(), lhs.type().id())) {
    ctx.messages.add(util::error_type_mismatch(op_symbol_, rhs.type(), lhs.type(), true));

    // add note if assigning to a pointer
    if (lhs.type().get_pointer()) {
      auto msg = std::make_unique<message::BasicMessage>(message::Note);
      msg->get() << "use the deference operator to update the value stored at the pointer";
      ctx.messages.add(std::move(msg));
    }

    return false;
  }

  // if type is zero-sized, skip as nothing happens
  if (lhs.type().size() == 0) return true;

  // coerce into correct type (this is safe as subtyping checked) and update our value
  const memory::Ref& expr = ctx.reg_alloc_manager.guarantee_datatype(rhs.rvalue().ref(), lhs.type());
  value_->rvalue(expr);

  // if symbol, assign symbol to register
  if (auto symbol = lhs.lvalue().get_symbol()) {
    ctx.symbols.assign_symbol(symbol->get().id(), expr.offset);

    // then evict it as it has been updated
    ctx.reg_alloc_manager.evict(symbol->get());
    return true;
  }

  // otherwise, treat as ptr
  auto lvalue_ref = lhs.lvalue().get_ref();
  assert(lvalue_ref);
  ctx.program.current().add(assembly::create_store(
      expr.offset,
      assembly::Arg::reg_indirect(lvalue_ref->get().offset)
    ));

  return true;
}

lang::ast::CastOperatorNode::CastOperatorNode(lang::lexer::Token token, const lang::ast::type::Node& target, std::unique_ptr<Node> expr)
: OperatorNode(token, token, {}), target_(target) {
  args_.push_back(std::move(expr));
  token_end(args_.back()->token_end());
}

bool lang::ast::CastOperatorNode::process(lang::Context& ctx) {
  // ensure target size is non-zero
  if (target_.size() == 0) {
    auto msg = lexer::TokenSpan::generate_message(message::Error);
    msg->get() << "attempted cast to zero-sized type ";
    target_.print_code(msg->get());
    ctx.messages.add(std::move(msg));
    return false;
  }

  if (!OperatorNode::process(ctx)) return false;
  value_ = value::rvalue(target_);
  return true;
}

bool lang::ast::CastOperatorNode::resolve_rvalue(lang::Context& ctx) {
  if (!get_arg_rvalue(ctx, 0)) return false;

  // fetch reference to arg and convert
  memory::Ref ref = args_.front()->value().rvalue().ref();
  ref = ctx.reg_alloc_manager.guarantee_datatype(ref, target_);

  // update rvalue
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

lang::ast::RegisterOfOperatorNode::RegisterOfOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> expr)
: OperatorNode(token, token, {}) {
  token_end(expr->token_end());
  args_.push_back(std::move(expr));
}

bool lang::ast::RegisterOfOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;
  value_ = value::rvalue(type::int32); // we will hold the register offset or -1
  return true;
}

std::ostream& lang::ast::RegisterOfOperatorNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << op_symbol_.image << "(";
  args_.front()->print_code(os);
  return os << ")";
}

bool lang::ast::RegisterOfOperatorNode::resolve_rvalue(lang::Context& ctx) {
  // get argument value, expect symbol
  auto& arg = args_.front();
  const value::Value& value = arg->value();
  if (!arg->resolve_value(ctx)) return {};
  // must be computable to a value (i.e., possible rvalue)
  if (!value.is_lvalue() || !value.lvalue().get_symbol()) {
    auto msg = arg->generate_message(message::Error);
    msg->get() << "expected symbol, got ";
    value.type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "while evaluating " << node_name();
    ctx.messages.add(std::move(msg));
    return {};
  }

  // hunt for the given symbol
  const symbol::Symbol& symbol = arg->value().lvalue().get_symbol()->get();
  auto maybe_ref = ctx.reg_alloc_manager.find(symbol);

  // get numerical register offset of symbol
  int offset = maybe_ref.has_value()
      ? ctx.reg_alloc_manager.guarantee_register(*maybe_ref).offset
      : -1;

  // load result into a register
  const memory::Literal& lit = memory::Literal::get(value_->type(), offset);
  memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(lit);
  value_->rvalue(std::make_unique<value::Literal>(lit, ref));

  return true;
}

lang::ast::AddressOfOperatorNode::AddressOfOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> expr)
    : OperatorNode(token, token, {}) {
  token_end(expr->token_end());
  args_.push_back(std::move(expr));
}

bool lang::ast::AddressOfOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;

  // get argument value, expect symbol
  Node& arg = *args_.front();
  const value::Value& value = arg.value();
  if (!arg.resolve_lvalue(ctx)) return {};
  // must be computable to a value (i.e., possible rvalue)
  if (!value.is_lvalue() || !value.lvalue().get_symbol()) {
    auto msg = arg.generate_message(message::Error);
    msg->get() << "expected symbol, got ";
    value.type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "while evaluating " << node_name();
    ctx.messages.add(std::move(msg));
    return false;
  }

  // get the symbol
  const symbol::Symbol& symbol = arg.value().lvalue().get_symbol()->get();
  symbol_ = symbol;

  // get the type and set our type
  auto& ptr_type = type::PointerNode::get(symbol.type());
  value_ = value::rvalue(ptr_type);
  return true;
}

bool lang::ast::AddressOfOperatorNode::resolve_rvalue(lang::Context& ctx) {
  assert(symbol_.has_value());

  // get location of the symbol (must have size then)
  const symbol::Symbol& symbol = symbol_->get();
  auto maybe_location = ctx.symbols.locate(symbol.id());

  if (!maybe_location) {
    auto msg = args_.front()->generate_message(message::Error);
    msg->get() << "symbol of type ";
    symbol.type().print_code(msg->get());
    msg->get() << " does not have an address";
    ctx.messages.add(std::move(msg));

    msg = symbol.token().generate_message(message::Note);
    msg->get() << "symbol defined here";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // find destination register
  memory::Ref ref = ctx.reg_alloc_manager.insert({nullptr});
  ref = ctx.reg_alloc_manager.guarantee_register(ref);

  // get address and store in register
  const memory::StorageLocation& location = maybe_location->get();

  if (location.type == memory::StorageLocation::Stack) { // calculate offset from $sp
    const uint64_t stack_offset = ctx.stack_manager.offset();

    if (location.offset == stack_offset) {
      ctx.program.current().add(assembly::create_load(
          ref.offset,
          assembly::Arg::reg(constants::registers::sp)
      ));
    } else if (location.offset < stack_offset) {
      ctx.program.current().add(assembly::create_sub(
          constants::inst::datatype::u64,
          ref.offset,
          constants::registers::sp,
          assembly::Arg::imm(stack_offset - location.offset)
      ));
    } else {
      ctx.program.current().add(assembly::create_add(
          constants::inst::datatype::u64,
          ref.offset,
          constants::registers::sp,
          assembly::Arg::imm(location.offset - location.offset)
      ));
    }
  } else {
    ctx.program.current().add(assembly::create_load(
        ref.offset,
        assembly::Arg::label(location.block)
    ));
  }

  // add comment
  auto& ptr_type = value_->type();
  auto& comment = ctx.program.current().back().comment();
  comment << op_symbol_.image << symbol.full_name() << ": ";
  ptr_type.print_code(comment);

  // update value_ & object
  memory::Object& object = ctx.reg_alloc_manager.find(ref);
  object.value = value::rvalue(ptr_type);
  object.value->rvalue(ref);
  value_->rvalue(std::make_unique<value::RValue>(ptr_type, ref));
  return true;
}

lang::ast::DereferenceOperatorNode::DereferenceOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> expr)
    : OperatorNode(token, token, {}) {
  token_end(expr->token_end());
  args_.push_back(std::move(expr));
}

bool lang::ast::DereferenceOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;

  // expect rvalue
  if (!get_arg_rvalue(ctx, 0)) return false;

  // must have a pointer type
  Node& arg = *args_.front();
  const auto ptr_type = arg.value().type().get_pointer();
  if (!ptr_type) {
    auto msg = arg.generate_message(message::Error);
    msg->get() << "expected pointer type, got ";
    arg.value().type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = op_symbol_.generate_message(message::Note);
    msg->get() << "while evaluating " << node_name();
    ctx.messages.add(std::move(msg));
    return false;
  }

  // get the inner type (pointer minus star)
  const auto& deref_type = ptr_type->unwrap();
  value_ = value::rlvalue(deref_type);
  return true;
}

bool lang::ast::DereferenceOperatorNode::resolve_lvalue(lang::Context& ctx) {
  Node& arg = *args_.front();

  // get the source (pointer location)
  memory::Ref src = arg.value().rvalue().ref();
  src = ctx.reg_alloc_manager.guarantee_register(src);

  // update value to point to the original
  value_->lvalue(src);

  return true;
}

bool lang::ast::DereferenceOperatorNode::resolve_rvalue(lang::Context& ctx) {
  if (!resolve_lvalue(ctx)) return false;

  Node& arg = *args_.front();
  const auto& deref_type = value_->type();

  // get the source (pointer location)
  const memory::Ref& src = value_->lvalue().get_ref()->get();

  // reserve a register to place the result
  memory::Ref dest = ctx.reg_alloc_manager.insert({nullptr});
  dest = ctx.reg_alloc_manager.guarantee_register(dest);

  // load into register
  ctx.program.current().add(assembly::create_load(
      dest.offset,
      assembly::Arg::reg_indirect(src.offset)
    ));

  // add comment
  auto& comment = ctx.program.current().back().comment();
  comment << "deref ";
  arg.value().type().print_code(comment);

  // update value_ & object
  memory::Object& object = ctx.reg_alloc_manager.find(dest);
  object.value = value::rvalue(deref_type);
  object.value->rvalue(dest);

  value_->rvalue(dest);

  return true;
}

lang::ast::FunctionCallOperatorNode::FunctionCallOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> subject, std::deque<std::unique_ptr<Node>> args)
  : OperatorNode(token, token, std::move(args)), subject_(std::move(subject)) {
  token_start(subject_->token_start());
  if (!args_.empty()) token_end(args_.back()->token_end());
}

bool lang::ast::FunctionCallOperatorNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  if (!subject_->collate_registry(messages, registry)) return false;
  for (auto& arg : args_) {
    if (!arg->collate_registry(messages, registry)) return false;
  }
  return true;
}

bool lang::ast::FunctionCallOperatorNode::process(lang::Context& ctx) {
  // process our subject and children
  if (!subject_->process(ctx) || !OperatorNode::process(ctx))
    return false;

  // get type of each argument
  std::deque<std::reference_wrapper<const type::Node>> arg_types;
  for (auto& arg : args_) {
    if (!arg->resolve_lvalue(ctx)) return false;
    arg_types.push_back(arg->value().type());
  }

  // if subject is a symbol ref...
  if (auto symbol_ref = subject_->value().get_symbol_ref()) {
    // get all possible symbols
    const std::string& name = symbol_ref->get();
    auto symbols = ctx.symbols.find(name);

    // if no symbols, this is easy
    if (symbols.empty()) {
      ctx.messages.add(util::error_unresolved_symbol(*subject_, name));
      return false;
    }

    // extract function types into an option list
    std::deque<std::reference_wrapper<const type::FunctionNode>> candidates;
    for (auto& symbol : symbols) {
      if (auto func_type = symbol.get().type().get_func())
        candidates.push_back(*func_type);
    }

    // if no candidates immediately, means it is not a function, so continue as normal
    if (!candidates.empty()) {
// filter to viable candidates
      candidates = type::FunctionNode::filter_candidates(arg_types, candidates);

      // if only one candidate remaining, this is good!
      if (candidates.size() == 1) {
        // update signature
        symbol::Symbol& symbol = ctx.symbols.find(name, candidates.front()).value();
        signature_ = candidates.front();
        func_name_ = name;

        // record function's location
        loc_ = ctx.symbols.locate(symbol.id());
        assert(loc_.has_value());

        value_ = value::rvalue(signature_->get().returns());
        return true;
      }

      // otherwise, we have insufficient information
      auto msg = generate_message(message::Error);
      msg->get() << "unable to resolve a suitable overload for " << name << "(";
      for (int i = 0; i < arg_types.size(); i++) {
        arg_types[i].get().print_code(msg->get());
        if (i + 1 < arg_types.size()) msg->get() << ", ";
      }
      msg->get() << ")";
      ctx.messages.add(std::move(msg));

      // print *function* candidates
      for (const symbol::Symbol& symbol : symbols) {
        if (auto func_type = symbol.type().get_func()) {
          msg = symbol.token().generate_message(message::Note);
          msg->get() << "candidate: " << symbol.full_name();
          func_type->print_code(msg->get());
          ctx.messages.add(std::move(msg));
        }
      }
      return false;
    }
  }

  // TODO what if value has function type?

  // otherwise, we need to hunt for an overload
  // first, evaluate our subject
  if (!subject_->resolve_rvalue(ctx)) return false;

  // add the subject's type to the signature
  arg_types.push_front(subject_->value().type());
  const auto& signature = type::FunctionNode::create(arg_types, std::nullopt);
  signature_ = signature;

  // try to find candidate
  auto op = ops::select_candidate("()", signature, *this, ctx.messages);
  if (!op.has_value()) return false;

  // make sure to replace with the correct signature (esp. for the return type)
  signature_ = op->get().type();
  value_ = value::rvalue(op->get().type().returns());

  assert(!op->get().builtin());
  if (op->get().builtin()) {
    // TODO what if op is built-in?
  } else {
    // get location of the operator
    auto& builtin = static_cast<const ops::UserDefinedOperator&>(op->get());
    loc_ = ctx.symbols.locate(builtin.symbol_id());
    assert(loc_.has_value());
  }

  return true;
}

bool lang::ast::FunctionCallOperatorNode::resolve_rvalue(lang::Context& ctx) {
  assert(loc_.has_value());

  if (!ops::call_function(
      *loc_,
      func_name_,
      *signature_,
      args_,
      *value_,
      ctx
    )) return false;

  // save registers
  ctx.reg_alloc_manager.save_store(true);

  // save $fp
  ctx.stack_manager.push_frame(true);

  // push arguments
  for (int i = 0; i < args_.size(); i++) {
    auto value = get_arg_rvalue(ctx, i);
    if (!value.has_value()) return false;

    // get location of the argument
    const memory::Ref location = value->get().rvalue().ref();

    // create necessary space on the stack
    size_t bytes = value->get().type().size();
    ctx.stack_manager.push(bytes);
    ctx.program.current().back().comment() << "arg #" << (i + 1);

    // store data in register here
    assembly::create_store(
        location.offset,
        assembly::Arg::reg_indirect(constants::registers::sp),
        bytes,
        ctx.program.current()
    );

    // we are done with this register now
    ctx.reg_alloc_manager.mark_free(location);
  }

  // call the function (via jal) and add comment
  auto arg = ctx.symbols.resolve_location(loc_->get());
  ctx.program.current().add(assembly::create_jump_and_link(std::move(arg)));
  auto& comment = ctx.program.current().back().comment();
  comment << "call " << func_name_;
  signature_->get().print_code(comment);

  // update value_'s location
  value_->rvalue(memory::Ref::reg(constants::registers::ret));

  // restore $sp
  ctx.stack_manager.pop_frame(true);

  // restore registers
  ctx.reg_alloc_manager.destroy_store(true);

  return true;
}

std::ostream& lang::ast::FunctionCallOperatorNode::print_code(std::ostream& os, unsigned int indent_level) const {
  subject_->print_code(os, indent_level);
  os << "(";

  for (int i = 0; i < args_.size(); i++) {
    args_[i]->print_code(os);
    if (i < args_.size() - 1) os << std::endl;
  }

  return os << ")";
}

std::ostream& lang::ast::FunctionCallOperatorNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);

  os << std::endl;
  subject_->print_tree(os, indent_level + 1);

  for (auto& arg : args_) {
    os << std::endl;
    arg->print_tree(os, indent_level + 1);
  }

  return os;
}
