#include "operator_definition.hpp"
#include "../block.hpp"
#include "context.hpp"
#include "operators/operator.hpp"
#include "operators/user_defined.hpp"
#include "operators/info.hpp"

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

bool lang::ast::OperatorDefinitionNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  // check if this operator can be overriden
  auto& op_info = info();
  if (!op_info.overloadable) {
    auto msg = name().generate_message(message::Error);
    msg->get() << (params() == 1 ? "unary" : params() > 2 ? "n-ary" : "binary") << " operator '" << name().image << "' is not overloadable";
    messages.add(std::move(msg));
    return false;
  }

  // check if operator overload already exists
  auto exists = ops::get(name().image, type_);
  if (exists.has_value()) {
    std::unique_ptr<message::BasicMessage> msg = name().generate_message(message::Error);
    msg->get() << "operator" << name().image;
    type_.print_code(msg->get(), false);
    msg->get() << " already exists";
    messages.add(std::move(msg));

    // point to previous location
    if (exists->get().builtin()) {
      msg = std::make_unique<message::BasicMessage>(message::Note);
      msg->get() << "previously defined as a built-in";
    } else {
      auto user_op = static_cast<const ops::UserDefinedOperator&>(exists->get());
      msg = user_op.symbol().token().generate_message(message::Note);
      msg->get() << "previously defined here";
    }
    messages.add(std::move(msg));

    return false;
  }

  // collate parent before definition
  if (!FunctionNode::collate_registry(messages, registry)) return false;

  // create operator entry
  ops::store_operator(std::make_unique<ops::UserDefinedOperator>(
      name().image,
      type(),
      registry.get(id())
  ));

  return true;
}
