#include <cassert>
#include "operator.hpp"
#include "shell.hpp"
#include "ast/types/function.hpp"
#include "operators/operator.hpp"
#include "context.hpp"
#include "ast/types/namespace.hpp"
#include "language/src/ast/leaves/symbol_reference.hpp"
#include "ast/namespace.hpp"
#include "message_helper.hpp"
#include "value/future.hpp"
#include "ast/types/graph.hpp"
#include "ast/types/int.hpp"
#include "ast/types/pointer.hpp"
#include "assembly/create.hpp"
#include "ast/function/function.hpp"
#include "operators/user_defined.hpp"
#include "operators/builtins.hpp"
#include "ast/types/bool.hpp"

std::string lang::ast::OperatorNode::node_name() const {
  return args_.size() == 2
    ? "binary operator"
    : "unary operator";
}

lang::ast::Node& lang::ast::OperatorNode::arg(int i) const {
  return *args_.at(i);
}

bool lang::ast::OperatorNode::expect_arg_lrvalue(int i, message::List& messages, bool expect_lvalue) const {
  auto& value = arg(i).value();
  if ((expect_lvalue && !value.is_lvalue()) || (!expect_lvalue && !value.is_rvalue())) {
    messages.add(util::error_expected_lrvalue(arg(i), value.type(), expect_lvalue));
    return false;
  }

  return true;
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

bool lang::ast::OverloadableOperatorNode::process(Context& ctx) {
  int args_start_index = 0;

  // special case: +/- on a pointer
  if ((op_symbol_.image == "+" || op_symbol_.image == "-") && args_.size() == 2) {
    // resolve first argument
    if (!arg(0).process(ctx) || !arg(0).resolve(ctx)) return false;
    args_start_index = 1;

    // check if pointer
    if (arg(0).value().type().get_pointer()) {
      arg(1).type_hint(type::uint64); // type hint as u64  for better arithmetic
      if (!arg(1).process(ctx) || !arg(1).resolve(ctx)) return false;
      args_start_index = 2;
      special_pointer_op_ = true;
    }
  }

  // process remaining arguments & accumulate types
  std::deque<std::reference_wrapper<const type::Node>> arg_types;
  for (int i = 0; i < args_.size(); i++) {
    if (i >= args_start_index && !arg(i).process(ctx)) return false;
    if (!arg(i).resolve(ctx)) return false;
    arg_types.push_back(arg(i).value().type());
  }
  const auto& signature = type::FunctionNode::create(arg_types, std::nullopt);
  signature_ = signature;

  // if special_pointer_op_, second arg must be an integer
  if (special_pointer_op_) {
    if (!type::graph.is_subtype(arg_types[1].get().id(), type::uint64.id())) {
      ctx.messages.add(util::error_type_mismatch(*this, arg_types[1].get(), type::uint64, false));
      return false;
    }

    // create value_
    value_ = value::value(arg_types.front().get());
    return true;
  }

  // try to find candidate
  auto maybe_op = ops::select_candidate(symbol(), signature, *this, ctx.messages);
  if (!maybe_op.has_value()) return false;
  op_ = maybe_op->get();

  // make sure to replace with the correct signature (esp. for the return type)
  signature_ = op_->get().type();

  // update rvalue return
  value_ = value::value(op_->get().type().returns());
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

bool lang::ast::OverloadableOperatorNode::generate_code(Context& ctx) {
  if (special_pointer_op_) {
    // generate code for arguments and check if rvalues
    if (!arg(0).generate_code(ctx)) return false;
    arg(0).value().materialise(ctx, arg(0).token_start().loc);
    if (!expect_arg_lrvalue(0, ctx.messages, false)) return false;

    if (!arg(1).generate_code(ctx)) return false;
    arg(1).value().materialise(ctx, arg(1).token_start().loc);
    if (!expect_arg_lrvalue(1, ctx.messages, false)) return false;

    // generate code for pointer arithmetic operation
    const int index = ctx.program.current().size();
    memory::Ref result = ops::pointer_arithmetic(ctx, arg(0).value(), arg(1).value(), op_symbol_.image == "-", true);

    // update location
    ctx.program.update_line_origins(op_symbol_.loc, index);

    // update value_
    value_->rvalue(result);
    return true;
  }

  // operator must have been set by ::process, so invoke it
  assert(op_.has_value());
  const int index = ctx.program.current().size();
  bool success = op_->get().invoke(
      ctx,
      args_,
      *value_,
      ops::InvocationOptions(conditional_context(), op_symbol_.loc)
  );

  // update location and return
  ctx.program.update_line_origins(op_symbol_.loc, index);
  return success;
}

std::unique_ptr<lang::ast::OperatorNode> lang::ast::OperatorNode::unary(lexer::Token token, std::unique_ptr<Node> expr) {
  lexer::Token symbol = token;

  // check for *special* operators
  if (token.type == lexer::TokenType::sizeof_kw) {
    return std::make_unique<SizeOfOperatorNode>(token, symbol, std::move(expr));
  } else if (token.image == "&") {
    return std::make_unique<AddressOfOperatorNode>(token, symbol, std::move(expr));
  } else if (token.image == "*") {
    return std::make_unique<DereferenceOperatorNode>(token, symbol, std::move(expr));
  }

  // otherwise, generic unary op
  std::deque<std::unique_ptr<Node>> children;
  children.push_back(std::move(expr));
  return std::make_unique<OverloadableOperatorNode>(token, symbol, std::move(children));
}

std::unique_ptr<lang::ast::OperatorNode>
lang::ast::OperatorNode::binary(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs) {
  // check for *special* operators
  if (symbol.image == ".") {
    return std::make_unique<DotOperatorNode>(std::move(token), std::move(symbol), std::move(lhs), std::move(rhs));
  } else if (symbol.image == "=") {
    return std::make_unique<AssignmentOperatorNode>(std::move(token), std::move(symbol), std::move(lhs), std::move(rhs));
  } else if (symbol.image == "[]") {
    return std::make_unique<SubscriptOperatorNode>(std::move(token), std::move(symbol), std::move(lhs), std::move(rhs));
  } else if (symbol.image == "&&" || symbol.image == "||") {
    return std::make_unique<LazyLogicalOperator>(std::move(token), std::move(symbol), std::move(lhs), std::move(rhs));
  }

  // otherwise, generic binary op
  std::deque<std::unique_ptr<Node>> children;
  children.push_back(std::move(lhs));
  children.push_back(std::move(rhs));
  return std::make_unique<OverloadableOperatorNode>(std::move(token), std::move(symbol), std::move(children));
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
  arg(0).type_hint(target_);
  if (!OperatorNode::process(ctx)) return false;
  value_ = value::value(target_);
  return true;
}

bool lang::ast::CStyleCastOperatorNode::generate_code(lang::Context& ctx) {
  if (!arg(0).resolve(ctx) || !arg(0).generate_code(ctx)) return false;
  auto& value = arg(0).value();
  value.materialise(ctx, arg(0).token_start().loc);
  if (!expect_arg_lrvalue(0, ctx.messages, false)) return false;

  // fetch reference to arg and convert
  memory::Ref ref = value.rvalue().ref();
  ref = ctx.reg_alloc_manager.guarantee_datatype(ref, target_);

  // update rvalue
  value_->rvalue(ref);
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
  // process children, resolve LHS
  if (!OperatorNode::process(ctx) || !arg(0).resolve(ctx)) return false;

  // expect RHS to be a symbol
  auto &rhs_value = arg(1).value();
  if (!rhs_value.get_symbol_ref()) {
    auto msg = op_symbol_.generate_message(message::Error);
    msg->get() << "expected symbol on RHS of member access, got ";
    rhs_value.type().print_code(msg->get());
    ctx.messages.add(std::move(msg));
    return false;
  }
  property_ = rhs_value.get_symbol_ref()->get();

  // if LHS is a namespace, we are symbol resolution
  auto &lhs_value = arg(0).value();
  if (lhs_value.type() == type::name_space) {
    // form combined attribute name
    const std::string new_name = lhs_value.lvalue().get_symbol()->get().full_name() + "." + property_;
    value_ = value::symbol_ref(new_name, ctx.symbols);
    return true;
  }

  // otherwise, check if the type has the given property
  if (auto prop_type = lhs_value.type().get_property_type(property_)) {
    value_ = value::value(prop_type);
    return true;
  }

  // otherwise, error
  auto msg = op_symbol_.generate_message(message::Error);
  msg->get() << "type ";
  lhs_value.type().print_code(msg->get());
  msg->get() << " has no property '" << property_ << "'";
  ctx.messages.add(std::move(msg));

  msg = arg(0).generate_message(message::Note);
  msg->get() << "subject of type ";
  lhs_value.type().print_code(msg->get()) << " appeared here";
  ctx.messages.add(std::move(msg));
  return false;
}

bool lang::ast::DotOperatorNode::resolve(Context& ctx) {
  // if we are a symbol reference, resolve it
  if (value_->get_symbol_ref()) {
    return static_cast<value::SymbolRef&>(*value_).resolve(*this, ctx.messages, type_hint());
  }

  return true;
}

bool lang::ast::DotOperatorNode::generate_code(lang::Context& ctx) {
  // if value_ was a symbol reference, we have been resolved to a symbol
  if (value_->get_symbol_ref()) {
    // get attached symbol (we have been resolved by this point)
    const value::Symbol* symbol = value().lvalue().get_symbol();
    assert(symbol != nullptr);

    // if size is 0, do nothing
    if (symbol->type().size() == 0) return true;

    // ensure the symbol is defined
    if (!symbol->get().define(ctx)) return false;

    // load into registers
    const int index = ctx.program.current().size();
    const memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(symbol->get());
    value_->rvalue(std::make_unique<value::RValue>(symbol->type(), ref));
    ctx.program.update_line_origins(arg(1).token_start().loc, index);
    return true;
  }

  // otherwise, get the property
  assert(!property_.empty());
  if (!(value_ = arg(0).value().type().get_property(ctx, property_))) {
    return false;
  }

  value_->materialise(ctx, arg(1).token_start().loc);
  if (!value_->is_rvalue()) {
    ctx.messages.add(util::error_expected_lrvalue(arg(0), value_->type(), false));
    return false;
  }

  // add comment
  auto& comment = ctx.program.current().back().comment();
  const auto copy = comment.str();
  comment.str("");
  comment << "." << property_;
  if (!copy.empty()) comment << " = " << copy;

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
  // process arguments, resolve LHS
  if (!OperatorNode::process(ctx) || !arg(0).resolve(ctx)) return false;

  // set value_ to LHS
  auto &lhs_value = arg(0).value();
  value_ = value::value(lhs_value.type());

  // check if LHS is a symbol
  if (lhs_value.is_lvalue()) {
    if (auto symbol_value = lhs_value.lvalue().get_symbol()) {
      const symbol::Symbol& symbol = symbol_value->get();

      // cannot assign to a constant outside of declaration
      if (symbol.is_constant()) {
        auto msg = op_symbol_.generate_message(message::Error);
        msg->get() << "unable to assign to constant symbol";
        ctx.messages.add(std::move(msg));

        msg = symbol.token().generate_message(message::Note);
        msg->get() << "symbol '" << symbol.full_name() << "' defined here";
        ctx.messages.add(std::move(msg));
        return false;
      }
    }
  }

  // resolve RHS
  if (!arg(1).resolve(ctx)) return false;
  auto& rhs_value = arg(1).value();

  // check that types are compatible
  if (!type::graph.is_subtype(rhs_value.type().id(), lhs_value.type().id())) {
    ctx.messages.add(util::error_type_mismatch(op_symbol_, rhs_value.type(), lhs_value.type(), true));

    // add note if assigning to a pointer
    if (lhs_value.type().get_pointer()) {
      auto msg = std::make_unique<message::BasicMessage>(message::Note);
      msg->get() << "use the deference operator to update the value stored at the pointer";
      ctx.messages.add(std::move(msg));
    }

    return false;
  }

  return true;
}

bool lang::ast::AssignmentOperatorNode::generate_code(lang::Context& ctx) {
  // generate RHS, assert it is an rvalue
  if (!arg(1).generate_code(ctx)) return false;
  auto& rhs_value = arg(1).value();
  rhs_value.materialise(ctx, arg(1).token_start().loc); // TODO get target from LHS?
  if (!expect_arg_lrvalue(1, ctx.messages, false)) return false;

  // generate LHS, assert it is an lvalue
  if (!arg(0).generate_code(ctx)) return false;
  auto& lhs_value = arg(0).value();
  if (!expect_arg_lrvalue(0, ctx.messages, true)) return false;

  // update value_ to point to LHS
  value_->lvalue(lhs_value.lvalue().copy());

  // if type is zero-sized, skip as nothing happens
  if (lhs_value.type().size() == 0) return true;

  // coerce into correct type (this is safe as subtyping checked) and update our value
  const int index = ctx.program.current().size();
  const memory::Ref& expr = ctx.reg_alloc_manager.guarantee_datatype(rhs_value.rvalue().ref(), lhs_value.type());
  value_->rvalue(expr);

  // if reference_as_ptr, copy them across
  if (rhs_value.type().reference_as_ptr()) {
    assert(lhs_value.type().reference_as_ptr()); // both should be ptr-like, should be handled by subtyping to ensure this
    assert(lhs_value.type().size() == rhs_value.type().size());

    // copy memory between the two
    ops::mem_copy(ctx, expr, lhs_value, nullptr);
  } else if (auto symbol = lhs_value.lvalue().get_symbol()) {
    // if symbol, assign symbol to register
    ctx.symbols.assign_symbol(symbol->get().id(), expr.offset);

    // then evict it as it has been updated
    ctx.reg_alloc_manager.evict(symbol->get());
  } else {
    // otherwise, treat as ptr
    auto lvalue_ref = lhs_value.lvalue().get_ref();
    assert(lvalue_ref);
    ctx.program.current().add(assembly::create_store(
        expr.offset,
        assembly::Arg::reg_indirect(lvalue_ref->get().offset)
    ));
  }

  // update origins
  ctx.program.update_line_origins(op_symbol_.loc, index);

  return true;
}

lang::ast::CastOperatorNode::CastOperatorNode(lang::lexer::Token token, lexer::Token symbol, const lang::ast::type::Node& target, std::unique_ptr<Node> expr, bool sudo)
: OperatorNode(token, symbol, {}), target_(target), sudo_(sudo) {
  args_.push_back(std::move(expr));
  token_end(args_.back()->token_end());
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

bool lang::ast::CastOperatorNode::process(lang::Context& ctx) {
  // ensure target size is non-zero
  if (target_.size() == 0) {
    auto msg = lexer::TokenSpan::generate_message(message::Error);
    msg->get() << "attempted cast to zero-sized type ";
    target_.print_code(msg->get());
    ctx.messages.add(std::move(msg));
    return false;
  }

  // otherwise, process child
  arg(0).type_hint(target_);
  if (!OperatorNode::process(ctx)) return false;

  // set value (we know type)
  value_ = value::value(target_);
  return true;
}

bool lang::ast::CastOperatorNode::generate_code(lang::Context& ctx) {
  // generate argument code & assure rvalue
  if (!arg(0).resolve(ctx) || !arg(0).generate_code(ctx)) return false;

  // get argument's original value
  auto& value = arg(0).value();
  auto& type = value.type();

  // materialise and ensure we have an rvalue
  value.materialise(ctx, arg(0).token_start().loc);
  if (!expect_arg_lrvalue(0, ctx.messages, false)) return false;

  // if not sudo, we cannot cast if:
  // - functional and do not match
  // - pointers to non-pointers
  if (!sudo_) {
    if ((target_.get_func() && !(type.get_pointer() || (type.get_func() && target_ == type)))
        || (target_.get_pointer() && !type.get_pointer() && !type.get_array())) {
      ctx.messages.add(util::error_type_mismatch(*this, type, target_, false));
      return false;
    }
  }

  // fetch reference to arg and convert
  const int index = ctx.program.current().size();
  memory::Ref ref = value.rvalue().ref();
  ref = ctx.reg_alloc_manager.guarantee_datatype(ref, target_);

  // update location
  ctx.program.update_line_origins(op_symbol_.loc, index);

  // update rvalue
  value_->rvalue(ref);
  return true;
}

lang::ast::AddressOfOperatorNode::AddressOfOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> expr)
    : OperatorNode(token, token, {}) {
  token_end(expr->token_end());
  args_.push_back(std::move(expr));
}

bool lang::ast::AddressOfOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;

  // get argument value
  Node& arg = this->arg(0);
  if (type_hint().has_value())  // pass type hint forward if we have one
    if (auto ptr = type_hint()->get().get_pointer())
      arg.type_hint(ptr->unwrap());
  const value::Value& value = arg.value();
  if (!arg.resolve(ctx)) return false;

  // if lvalue/reference-like
  if (value.is_lvalue() || value.type().reference_as_ptr()) {
    // set our type to the wrapped input type
    auto& ptr_type = type::PointerNode::get(value.type());
    value_ = value::value(ptr_type);
    return true;
  }

  // error otherwise
  auto msg = arg.generate_message(message::Error);
  msg->get() << "expected lvalue, got ";
  value.type().print_code(msg->get());
  ctx.messages.add(std::move(msg));

  msg = op_symbol_.generate_message(message::Note);
  msg->get() << "while evaluating " << node_name();
  ctx.messages.add(std::move(msg));
  return false;
}

bool lang::ast::AddressOfOperatorNode::generate_code(lang::Context& ctx) {
  // get the argument's value
  auto& value = arg(0).value();
  const int index = ctx.program.current().size();

  // if we are a symbol...
  if (value.is_lvalue() && value.lvalue().get_symbol()) {
    // get our symbol
    const symbol::Symbol& symbol = value.lvalue().get_symbol()->get();

    // attempt to get the address of
    bool success = ops::address_of(ctx, symbol, *value_);
    ctx.program.update_line_origins(op_symbol_.loc, index);

    // if error, this means no location
    if (!success) {
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

    // add comment
    auto& comment = ctx.program.current().back().comment();
    comment << op_symbol_.image << symbol.full_name() << ": ";
    value_->type().print_code(comment);

    return true;
  }

  // otherwise, we will use a reference
  // generate symbol's code
  if (!arg(0).generate_code(ctx)) return false;

  value.materialise(ctx, arg(0).token_start().loc);
  const memory::Ref& ref = value.is_lvalue() && value.lvalue().get_ref()
      ? value.lvalue().get_ref()->get()
      : value.rvalue().ref();

  // update value_ & object
  memory::Object& object = ctx.reg_alloc_manager.find(ref);
  ctx.program.update_line_origins(op_symbol_.loc, index);
  object.value = value::rvalue(value_->type(), ref);
  value_->rvalue(ref);

  return true;
}

lang::ast::DereferenceOperatorNode::DereferenceOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> expr)
    : OperatorNode(token, token, {}) {
  token_end(expr->token_end());
  args_.push_back(std::move(expr));
}

bool lang::ast::DereferenceOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx) || !arg(0).resolve(ctx)) return false;

  // must have a pointer type
  Node& arg = *args_.front();

  // expect array or pointer type
  const auto wrapper_type = arg.value().type().get_wrapper();
  if (wrapper_type && (wrapper_type->get_pointer() || wrapper_type->get_array())) {
    value_ = value::value(wrapper_type->unwrap());
    return true;
  }

  auto msg = arg.generate_message(message::Error);
  msg->get() << "expected pointer or array type, got ";
  arg.value().type().print_code(msg->get());
  ctx.messages.add(std::move(msg));

  msg = op_symbol_.generate_message(message::Note);
  msg->get() << "cannot dereference a non-pointer value";
  ctx.messages.add(std::move(msg));
  return false;
}

bool lang::ast::DereferenceOperatorNode::generate_code(lang::Context& ctx) {
  // generate argument, expect rvalue
  if (!arg(0).generate_code(ctx)) return false;
  auto& pointer_value = arg(0).value();
  pointer_value.materialise(ctx, arg(0).token_start().loc);
  if (!expect_arg_lrvalue(0, ctx.messages, false)) return false;

  // get the source (pointer location)
  const int index = ctx.program.current().size();
  memory::Ref src = pointer_value.rvalue().ref();
  src = ctx.reg_alloc_manager.guarantee_register(src);

  // update value to point to the original
  value_->lvalue(src);

  // dereference the pointer
  ops::dereference(ctx, pointer_value, *value_, true);

  // update lines' origin
  ctx.program.update_line_origins(op_symbol_.loc, index);

  return true;
}

lang::ast::FunctionCallOperatorNode::FunctionCallOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> subject, std::deque<std::unique_ptr<Node>> args)
  : OperatorNode(std::move(token), std::move(symbol), std::move(args)), subject_(std::move(subject)) {
  token_start(subject_->token_start());
  if (!args_.empty()) token_end(args_.back()->token_end());
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

  // if we have only one possible overload, use that
  if (auto symbol_ref = subject_->value().get_symbol_ref(); symbol_ref && symbol_ref->candidates().size() == 1) {
    const symbol::Symbol& symbol = symbol_ref->candidates().front();

    // first, check if of function type, otherwise, continue
    if (auto func_type = symbol.type().get_func()) {
      // check if #args matches
      if (func_type->args() != args_.size()) {
        auto msg = generate_message(message::Error);
        msg->get() << "expected " << func_type->args() << " argument";
        if (func_type->args() != 1) msg->get() << "s";
        msg->get() << ", got " << args_.size();
        ctx.messages.add(std::move(msg));

        msg = symbol.token().generate_message(message::Note);
        msg->get() << "function '" << symbol.full_name() << "' defined here";
        ctx.messages.add(std::move(msg));

        return false;
      }

      // resolve argument types (lvalues, as rvalues are done later)
      int i = 0;
      for (auto& arg : args_) {
        arg->type_hint(func_type->arg(i)); // provide a type hint
        if (!arg->resolve(ctx)) return false;

        // check that types match
        if (!type::graph.is_subtype(arg->value().type().id(), func_type->arg(i).id())) {
          ctx.messages.add(util::error_type_mismatch(*arg, arg->value().type(), func_type->arg(i), false));

          auto msg = symbol.token().generate_message(message::Note);
          msg->get() << "function ";
          func_type->print_code(msg->get()) << " defined here";
          ctx.messages.add(std::move(msg));
          return false;
        }

        i++;
      }

      signature_ = *func_type;
      value_ = value::value(func_type->returns());
      symbol_ = symbol;

      return true;
    }
  }

  // get type of each argument
  std::deque<std::reference_wrapper<const type::Node>> arg_types;
  for (auto& arg : args_) {
    if (!arg->resolve(ctx)) return false;
    arg_types.push_back(arg->value().type());
  }

  // if subject is a symbol ref...
  if (auto symbol_ref = subject_->value().get_symbol_ref()) {
    // get all possible symbols
    const std::string& name = symbol_ref->get();
    auto symbols = ctx.symbols.find(name);

    // if no symbols, this is easy
    if (symbols.empty()) {
      ctx.messages.add(util::error_symbol_not_found(*subject_, name));
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
        value_ = value::value(signature_->get().returns());
        symbol_ = symbol;

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

  // otherwise, we need to hunt for an overload
  // first, evaluate our subject
  if (!subject_->resolve(ctx)) return false;

  // what if the subject has a functional type signature?
  if (auto func_type = subject_->value().type().get_func()) {
    signature_ = *func_type;
    value_ = value::value(func_type->returns());
    return true;
  }

  // add the subject's type to the signature
  arg_types.push_front(subject_->value().type());
  const auto& signature = type::FunctionNode::create(arg_types, std::nullopt);
  signature_ = signature;

  // try to find candidate
  auto op = ops::select_candidate("()", signature, *this, ctx.messages);
  if (!op.has_value()) return false;

  // make sure to replace with the correct signature (esp. for the return type)
  signature_ = op->get().type();
  value_ = value::value(op->get().type().returns());

  assert(!op->get().builtin());
  if (op->get().builtin()) {
    // TODO what if op is built-in?
  } else {
    // get location of the operator
    auto& user_op = static_cast<const ops::UserDefinedOperator&>(op->get());
    symbol_ = user_op.symbol();
  }

  return true;
}

bool lang::ast::FunctionCallOperatorNode::generate_code(lang::Context& ctx) {
  std::unique_ptr<assembly::BaseArg> arg;
  const int index = ctx.program.current().size();

  if (symbol_.has_value()) {
    // if we resolved to a symbol, make sure it is defined before looking up its location
    auto& symbol = symbol_->get();
    if (!symbol.define(ctx)) return false;

    // get the symbol's location
    auto location = ctx.symbols.locate(symbol.id());
    assert(location.has_value());

    // if we are a function, call its location
    if (symbol.category() == symbol::Category::Function) {
      arg = location->get().resolve();
    } else {
      // otherwise, load and dereference it
      memory::Ref ref = ctx.reg_alloc_manager.insert({value::value(symbol.type())});
      ref = ctx.reg_alloc_manager.guarantee_register(ref);

      ctx.program.current().add(assembly::create_load(
          ref.offset,
          location->get().resolve()
        ));
      ctx.program.current().back().origin(subject_->token_start().loc);

      auto& comment = ctx.program.current().back().comment();
      comment << symbol.full_name() << ": ";
      symbol.type().print_code(comment);

      arg = assembly::Arg::reg_indirect(ref.offset);
      ctx.reg_alloc_manager.mark_free(ref);
    }
  } else {
    // otherwise, generate subject's code (must be an rvalue)
    if (!subject_->generate_code(ctx)) return false;
    auto& value = subject_->value();
    value.materialise(ctx, subject_->token_start().loc);
    if (!value.is_rvalue()) {
      ctx.messages.add(util::error_expected_lrvalue(*subject_, value.type(), false));
     return false;
    }

    // load into a register (functional types are always rvalues)
    const memory::Ref ref = ctx.reg_alloc_manager.guarantee_register(value.rvalue().ref());
    arg = assembly::Arg::reg(ref.offset);
  }

  bool success = ops::call_function(
      std::move(arg),
      symbol_.has_value() ? symbol_->get().full_name() : "<expr>",
      *signature_,
      args_,
      {},
      *value_,
      target(),
      ctx
  );

  // update line origins
  ctx.program.update_line_origins(op_symbol_.loc, index);
  return success;
}

lang::ast::SizeOfOperatorNode::SizeOfOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> expr)
    : OperatorNode(token, token, {}) {
  token_end(expr->token_end());
  args_.push_back(std::move(expr));
}

lang::ast::SizeOfOperatorNode::SizeOfOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, const type::Node& type)
    : OperatorNode(token, token, {}), type_(type) {}

std::ostream& lang::ast::SizeOfOperatorNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << op_symbol_.image << "(";
  args_.front()->print_code(os);
  return os << ")";
}

bool lang::ast::SizeOfOperatorNode::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;
  value_ = value::value(type::uint64);
  return true;
}

bool lang::ast::SizeOfOperatorNode::generate_code(lang::Context& ctx) {
  optional_ref<const type::Node> type;
  const int index = ctx.program.current().size();

  // if a type is supplied, cool
  if (type_.has_value()) {
    type = type_;
  } else {
    // generate argument & get its type
    Node& arg = this->arg(0);
    if (!arg.resolve(ctx) || !arg.generate_code(ctx)) return false;
    type = arg.value().type();
  }

  // load the size of the type into a register
  const memory::Literal& lit = memory::Literal::get(value_->type(), type->get().size());
  memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(lit);
//  value_ = value::literal(lit, ref);
  value_->rvalue(ref); // TODO mark as literal somehow?

  // add comment
  auto& comment = ctx.program.current().back().comment();
  comment.str(""); // make sure it is cleared
  comment << "sizeof(";
  type->get().print_code(comment) << ")";

  // update line origins
  ctx.program.update_line_origins(op_symbol_.loc, index);

  return true;
}

lang::ast::SubscriptOperatorNode::SubscriptOperatorNode(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
    : OperatorNode(std::move(token), std::move(symbol), {}) {
  token_start(lhs->token_start());
  token_end(rhs->token_end());

  args_.push_back(std::move(lhs));
  args_.push_back(std::move(rhs));
}

std::ostream& lang::ast::SubscriptOperatorNode::print_code(std::ostream& os, unsigned int indent_level) const {
  arg(0).print_code(os, indent_level) << "[";
  arg(1).print_code(os, indent_level) << "]";
  return os;
}

bool lang::ast::SubscriptOperatorNode::process(lang::Context& ctx) {
  // process LHS first
  auto& lhs = arg(0);
  if (!lhs.process(ctx) || !lhs.resolve(ctx)) return false;

  // check if pointer/array
  // if so, we can provide a type hint to the rhs
  auto& rhs = arg(1);
  const auto lhs_wrapper = lhs.value().type().get_wrapper();
  const bool is_ptr_or_arr = lhs_wrapper->get_pointer() || lhs_wrapper->get_array();
  if (is_ptr_or_arr) {
    rhs.type_hint(type::uint64);
  }

  // process RHS finally
  if (!rhs.process(ctx) || !rhs.resolve(ctx)) return false;

  // if pointer/array, this is a built-in
  if (is_ptr_or_arr && rhs.value().type().get_int()) {
    value_ = value::value(lhs_wrapper->unwrap());
    return true;
  }

  // otherwise, hunt for an overload - create signature
  const auto& signature = type::FunctionNode::create({lhs.value().type(), rhs.value().type()}, std::nullopt);
  signature_ = signature;

  // try to find candidate
  op_ = ops::select_candidate("[]", signature, *this, ctx.messages);
  if (!op_.has_value()) return false;

  // make sure to replace with the correct signature (esp. for the return type)
  signature_ = op_->get().type();
  value_ = value::value(op_->get().type().returns());

  return true;
}

bool lang::ast::SubscriptOperatorNode::generate_code(lang::Context& ctx) {
  // check if we have a pointer/array LHS
  if (auto& lhs = arg(0).value(); lhs.type().get_pointer() || lhs.type().get_array()) {
    // generate both arguments, check if rvalues
    if (!arg(0).generate_code(ctx)) return false;
    arg(0).value().materialise(ctx, arg(0).token_start().loc);
    if (!expect_arg_lrvalue(0, ctx.messages, false)) return false;

    if (!arg(1).generate_code(ctx)) return false;
    arg(1).value().materialise(ctx, arg(1).token_start().loc);
    if (!expect_arg_lrvalue(1, ctx.messages, false)) return false;

    const int index = ctx.program.current().size();

    // perform pointer addition to get the source
    memory::Ref src = ops::pointer_arithmetic(ctx, lhs, arg(1).value(), false, false);
    src = ctx.reg_alloc_manager.guarantee_register(src);

    // update value to point to the original
    value_->lvalue(src);

    // dereference the result
    ops::dereference(ctx, lhs, *value_, false);

    // add comment
    auto& comment = ctx.program.current()[index].comment();
    comment << "operator" << op_symbol_.image << "(";
    arg(0).value().type().print_code(comment) << ", ";
    arg(1).value().type().print_code(comment) << ")";

    // update line origins
    ctx.program.update_line_origins(op_symbol_.loc, index);

    return true;
  }

  // otherwise, invoke op_
  assert(op_.has_value());
  return op_->get().invoke(
      ctx,
      args_,
      *value_,
      {conditional_context(), op_symbol_.loc}
  );
}

lang::ast::LazyLogicalOperator::LazyLogicalOperator(lang::lexer::Token token, lang::lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
    : OperatorNode(token, std::move(symbol), {}), and_(token.image == "&&") {
  token_start(lhs->token_start());
  token_end(rhs->token_end());

  args_.push_back(std::move(lhs));
  args_.push_back(std::move(rhs));
}

bool lang::ast::LazyLogicalOperator::process(lang::Context& ctx) {
  if (!OperatorNode::process(ctx)) return false;

  // both sides must be Booleans, but not necessarily rvalues
  for (int i = 0; i < 2; i++) {
    auto& arg = this->arg(i);
    if (!arg.resolve(ctx)) return false;
    if (!ast::type::graph.is_subtype(arg.value().type().id(), ast::type::boolean.id())) {
      ctx.messages.add(util::error_type_mismatch(arg, arg.value().type(), ast::type::boolean, false));
      return false;
    }
  }

  // set our value
  value_ = value::value(ast::type::boolean);

  return true;
}

bool lang::ast::LazyLogicalOperator::generate_code(lang::Context& ctx) {
  const ops::Operator& op = ops::get(op_symbol_.image).front();
  return op.invoke(ctx, args_, *value_, {conditional_context(), op_symbol_.loc});
}
