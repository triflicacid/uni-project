#include <cassert>
#include "conditional_context.hpp"
#include "assembly/create.hpp"
#include "value/value.hpp"
#include "context.hpp"
#include "message_helper.hpp"
#include "ast/types/graph.hpp"
#include "ast/types/bool.hpp"

lang::control_flow::ConditionalContext lang::control_flow::ConditionalContext::copy() const {
  return ConditionalContext{
      if_true, if_false, inverse_cond
  };
}

lang::control_flow::ConditionalContext lang::control_flow::ConditionalContext::inverse() const {
  return ConditionalContext{
      if_true, if_false, !inverse_cond
  };
}

void lang::control_flow::ConditionalContext::generate_branches(assembly::BasicBlock& block, constants::cmp::flag flag) {
  assert(!handled);

  // take inverse of the given flag if required
  const auto guard = inverse_cond
                     ? constants::cmp::inverse_of(flag)
                     : flag;

  if (if_true.has_value()) {
    block.add(assembly::create_branch(
        guard,
        assembly::Arg::label(if_true->get())
    ));
  }

  if (if_false.has_value()) {
    block.add(assembly::create_branch(
        constants::cmp::inverse_of(guard),
        assembly::Arg::label(if_false->get())
    ));
  }

  // update the handled flag so we only run once
  handled = true;
}

void lang::control_flow::ConditionalContext::generate_branches(lang::assembly::BasicBlock& block, uint8_t reg) {
  assert(!handled);

  // perform zero-comparison
  block.add(assembly::create_comparison(
      reg,
      assembly::Arg::imm(0)
  ));

  // generate branches
  generate_branches(block, constants::cmp::neq);
}

bool lang::control_flow::ConditionalContext::generate_branches(lang::Context& ctx, const message::MessageGenerator& source, const value::Value& value) {
  assert(!handled);

  // expect guard's result to be an rvalue
  if (!value.is_rvalue()) {
    ctx.messages.add(util::error_expected_lrvalue(source, value.type(), false));
    return false;
  }

  // the result must be a Boolean
  if (!ast::type::graph.is_subtype(value.type().id(), ast::type::boolean.id())) {
    ctx.messages.add(util::error_type_mismatch(source, value.type(), ast::type::boolean, false));
    return false;
  }

  // get reference to result, ensuring we are a Boolean
  memory::Ref result = value.rvalue().ref();
  result = ctx.reg_alloc_manager.guarantee_datatype(result, ast::type::boolean);

  // generate branches
  generate_branches(ctx.program.current(), result.offset);
  return true;
}

void lang::control_flow::ConditionalContext::flip_blocks() {
  auto tmp_true = if_true;
  if_true = if_false;
  if_false = tmp_true;
}
