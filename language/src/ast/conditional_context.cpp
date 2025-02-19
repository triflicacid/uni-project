#include <cassert>
#include "conditional_context.hpp"
#include "assembly/create.hpp"

lang::ast::ConditionalContext lang::ast::ConditionalContext::copy() const {
  return ConditionalContext{
      if_true, if_false, inverse_cond
  };
}

lang::ast::ConditionalContext lang::ast::ConditionalContext::inverse() const {
  return ConditionalContext{
      if_true, if_false, !inverse_cond
  };
}

void lang::ast::ConditionalContext::generate_branches(assembly::BasicBlock& block, constants::cmp::flag flag) {
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

  // update the handled flag so we're only run once
  handled = true;
}
