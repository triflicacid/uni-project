#pragma once

#include "optional_ref.hpp"
#include "assembly/basic_block.hpp"
#include "constants.hpp"

namespace lang::ast {
  // used for tracking the processing of conditional statements
  struct ConditionalContext {
    optional_ref<assembly::BasicBlock> if_true; // jump here if true, otherwise fall through
    optional_ref<assembly::BasicBlock> if_false; // jump here if false, otherwise fall through
    bool inverse_cond = false; // inverse conditional flag? `true` and `false` branches still maintained
    bool handled = false; // has this conditional been handled? I.e., have branches been handled (conditional vs expr)

    ConditionalContext copy() const;

    // return this, but with the inverse conditional
    ConditionalContext inverse() const;

    // generate branching instructions based on conditional flag
    // branch to `if_true` if flag is met, otherwise `if_false`
    // does not execute if `handled=true`
    // updates `handled` on execution
    void generate_branches(assembly::BasicBlock& block, constants::cmp::flag flag);
  };
}
