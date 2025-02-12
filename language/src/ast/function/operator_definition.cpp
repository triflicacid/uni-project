#include "operator_definition.hpp"
#include "../block.hpp"
#include "context.hpp"
#include "operators/operator.hpp"
#include "operators/user_defined.hpp"
#include "operators/info.hpp"

bool lang::ast::OperatorDefinitionNode::_process(lang::Context& ctx) {
  // check if this operator can be overriden
  auto& op_info = info();
  if (!op_info.overloadable) {
    auto msg = name().generate_message(message::Error);
    msg->get() << (params() == 1 ? "unary" : params() > 2 ? "n-ary" : "binary") << " operator '" << name().image << "' is not overloadable";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // check if operator overload already exists
  auto exists = ops::get(name().image, type_);
  if (exists.has_value()) {
    std::unique_ptr<message::BasicMessage> msg = name().generate_message(message::Error);
    msg->get() << "operator" << name().image;
    type_.print_code(msg->get(), false);
    msg->get() << " already exists";
    ctx.messages.add(std::move(msg));

    // point to previous location
    if (exists->get().builtin()) {
      msg = std::make_unique<message::BasicMessage>(message::Note);
      msg->get() << "previously defined as a built-in";
    } else {
      auto user_op = static_cast<const ops::UserDefinedOperator&>(exists->get());
      const symbol::Symbol& symbol = ctx.symbols.get(user_op.symbol_id());

      msg = symbol.token().generate_message(message::Note);
      msg->get() << "previously defined here";
    }
    ctx.messages.add(std::move(msg));

    return false;
  }

  // process parent first
  if (!FunctionNode::_process(ctx)) return false;

  // create operator entry
  ops::store_operator(std::make_unique<ops::UserDefinedOperator>(
      name().image,
      type(),
      id()
  ));

  return true;
}

const lang::ops::OperatorInfo& lang::ast::OperatorDefinitionNode::info() const {
  const std::string& name = this->name().image;
  switch (params()) {
    case 1:
      return ops::builtin_unary.contains(name)
        ? ops::builtin_unary.at(name)
        : ops::generic_unary;
    case 2:
      return ops::builtin_binary.contains(name)
             ? ops::builtin_binary.at(name)
             : ops::generic_binary;
   default:
    return ops::generic_binary;
  }
}
