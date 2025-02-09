#include "return.hpp"
#include "ast/types/graph.hpp"
#include "context.hpp"
#include "ast/function.hpp"
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
    : value::unit_value;
}

bool lang::ast::ReturnNode::process(lang::Context& ctx) {
  const type::Node* type; // store our type

  if (expr_.has_value()) {
    // process expressions if needs be
    if (!expr_.value()->process(ctx)) return false;

    // expect rvalue
    if (value().is_future_rvalue() && !expr_.value()->resolve_rvalue(ctx)) return false;
    type = &value().type();
    if (!value().is_rvalue()) {
      auto msg = expr_.value()->generate_message(message::Error);
      msg->get() << "expected rvalue, got ";
      type->print_code(msg->get());
      ctx.messages.add(std::move(msg));
      return false;
    }
  } else {
    type = &type::unit;
  }

  // get return type of the function
  const auto& function = ctx.symbols.current_function();
  assert(function.has_value());
  auto& return_type = function->get().type().returns();

  // check that types match
  if (!type::graph.is_subtype(type->id(), return_type.id())) {
    const Node& source = expr_.has_value()
        ? *expr_->get()
        : *this;
    ctx.messages.add(util::error_type_mismatch(source, *type, return_type, false));

    auto& symbol = ctx.symbols.get(function->get().id());
    auto msg = symbol.token().generate_message(message::Note);
    msg->get() << "enclosing function defined here";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // if expression
  if (!expr_.has_value()) {
    ctx.program.current().add(assembly::create_return());
    return true;
  }

  // otherwise, return a value
  memory::Ref ref = ctx.reg_alloc_manager.guarantee_register(value().rvalue().ref());
  ctx.program.current().add(assembly::create_return(assembly::Arg::reg(ref.offset)));
  return true;
}
