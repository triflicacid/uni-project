#pragma once

#include "optional_ref.hpp"
#include "assembly/basic_block.hpp"
#include "constants.hpp"

namespace message {
  class MessageGenerator;
}

namespace lang {
  struct Context;

  namespace value {
    class Value;
  }
}

namespace lang::control_flow {
  // used for tracking the processing of conditional statements
  struct ConditionalContext {
    optional_ref<assembly::BasicBlock> if_true; // jump here if true, otherwise fall through
    optional_ref<assembly::BasicBlock> if_false; // jump here if false, otherwise fall through
    bool inverse_cond = false; // inverse conditional flag? `true` and `false` branches still maintained
    bool handled = false; // has this conditional been handled? I.e., have branches been handled (conditional vs expr)

    ConditionalContext copy() const;

    // return this, but with the inverse conditional
    ConditionalContext inverse() const;

    // flip the order of the blocks
    void flip_blocks();

    // generate branching instructions based on conditional flag
    // branch to `if_true` if flag is met, otherwise `if_false`
    // does not execute if `handled=true`
    // updates `handled` on execution
    void generate_branches(assembly::BasicBlock& block, constants::cmp::flag flag);

    // optionally, generate branches based of a zero-comparison (or non-zero, if inverse_cond)
    // that is, go to if_true if != 0, if_false otherwise
    void generate_branches(assembly::BasicBlock& block, uint8_t reg);

    // handle the context given an input value, return if success
    bool generate_branches(Context& ctx, const message::MessageGenerator& source, const value::Value& value);
  };
}
