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
  /**
   * @brief Tracks the in-progress code generation of a conditional construct (if/while guard, logical operator) in terms of its true/false branch targets.
   */
  struct ConditionalContext {
    optional_ref<assembly::BasicBlock> if_true; // jump here if true, otherwise fall through
    optional_ref<assembly::BasicBlock> if_false; // jump here if false, otherwise fall through
    bool inverse_cond = false; // inverse conditional flag? `true` and `false` branches still maintained
    bool handled = false; // has this conditional been handled? I.e., have branches been handled (conditional vs expr)

    /**
     * @brief Creates an independent copy of this context with a reset handled flag.
     * @return The copied context.
     */
    ConditionalContext copy() const;

    /**
     * @brief Returns a copy of this context with the condition polarity inverted.
     * @return The inverted context, with the same target blocks but flipped inverse_cond and a reset handled flag.
     */
    // return this, but with the inverse conditional
    ConditionalContext inverse() const;

    /**
     * @brief Swaps the if_true and if_false target blocks in place.
     */
    // flip the order of the blocks
    void flip_blocks();

    /**
     * @brief Emits the conditional-branch instructions for a comparison flag already established in the given block.
     * @param block Basic block to append the branch instructions to.
     * @param flag Comparison flag to branch on (inverted first if inverse_cond is set).
     */
    // generate branching instructions based on conditional flag
    // branch to `if_true` if flag is met, otherwise `if_false`
    // does not execute if `handled=true`
    // updates `handled` on execution
    void generate_branches(assembly::BasicBlock& block, constants::cmp::flag flag);

    /**
     * @brief Emits a zero-comparison against a register followed by the corresponding conditional branches.
     * @param block Basic block to append the comparison and branch instructions to.
     * @param reg Register offset holding the value to compare against zero.
     */
    // optionally, generate branches based of a zero-comparison (or non-zero, if inverse_cond)
    // that is, go to if_true if != 0, if_false otherwise
    void generate_branches(assembly::BasicBlock& block, uint8_t reg);

    /**
     * @brief Validates a resolved value as a boolean condition and emits the corresponding branch instructions.
     * @param ctx Compiler context providing the message list, program, and register allocator.
     * @param source Location to attribute any diagnostics to.
     * @param value Resolved guard value to test.
     * @return True on success, false if the value is not a usable boolean r-value (in which case a diagnostic is added).
     */
    // handle the context given an input value, return if success
    bool generate_branches(Context& ctx, const message::MessageGenerator& source, const value::Value& value);
  };
}
