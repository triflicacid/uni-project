#include "return.hpp"
#include "ast/types/graph.hpp"
#include "context.hpp"
#include "function.hpp"
#include "message_helper.hpp"
#include "assembly/create.hpp"

std::ostream& lang::ast::ReturnNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "return";
  if (expr_.has_value()) {
    os << " ";
    expr_.value()->print_code(os);
  }
  return os;
}

std::ostream& lang::ast::ReturnNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  if (expr_.has_value()) {
    os << std::endl;
    expr_.value()->print_tree(os, indent_level + 1);
  }
  return os;
}

const lang::value::Value& lang::ast::ReturnNode::value() const {
  return expr_.has_value()
    ? expr_.value()->value()
    : *value::unit_value_instance;
}

bool lang::ast::ReturnNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  return !expr_.has_value() || expr_.value()->collate_registry(messages, registry);
}

bool lang::ast::ReturnNode::process(lang::Context& ctx) {
  // a return statement *must* be inside a function
  const auto& function = ctx.symbols.current_function();
  if (!function.has_value()) {
    auto msg = token_start().generate_message(message::Error);
    msg->get() << "return statement must be inside a function definition";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // lookup the function's return type
  auto& return_type = function->get().type().returns();
  const type::Node* type; // store our type

  if (expr_.has_value()) {
    // process expressions if needs be
    expr_.value()->type_hint(return_type);
    if (!expr_.value()->process(ctx)) return false;

    // provide type hint and resolve
    expr_.value()->type_hint(function->get().type().returns()); // give return value a type hint
    if (!expr_.value()->resolve(ctx)) return false;
    type = &value().type();
  } else {
    type = &type::unit;
  }

  // check that expr type match with return type
  if (!type::graph.is_subtype(type->id(), return_type.id())) {
    const Node& source = expr_.has_value() ? *expr_->get() : *this;
    ctx.messages.add(util::error_type_mismatch(source, *type, return_type, false));

    auto& symbol = ctx.symbols.get(function->get().id());
    auto msg = symbol.token().generate_message(message::Note);
    msg->get() << "enclosing function returns type ";
    return_type.print_code(msg->get());
    ctx.messages.add(std::move(msg));
    return false;
  }

  // set our value_
  value_ = value::value(*type);

  return true;
}

bool lang::ast::ReturnNode::generate_code(lang::Context& ctx) const {
  // if we have an expression, generate its code first
  if (expr_.has_value() && !expr_.value()->generate_code(ctx)) return false;

  // if we return nothing, or return a zero-sized type, generate generic `ret`
  if (!expr_.has_value() || value().type().size() == 0) {
    ctx.program.current().add(assembly::create_return());
    return true;
  }

  // otherwise, return a value
  memory::Ref ref = ctx.reg_alloc_manager.guarantee_register(value().rvalue().ref());
  ctx.program.current().add(assembly::create_return(assembly::Arg::reg(ref.offset)));

  return true;
}
