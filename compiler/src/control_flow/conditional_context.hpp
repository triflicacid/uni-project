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

/** @brief Context objects tracking the compiler's current conditional and loop nesting, used to fuse branches and resolve break/continue targets during code generation. */
namespace lang::control_flow {
  /**
   * @brief Tracks the in-progress code generation of a conditional construct (if/while guard, logical operator) in terms of its true/false branch targets.
   */
  struct ConditionalContext {
    optional_ref<assembly::BasicBlock> if_true; ///< Block to jump to if the condition is true; falls through if empty.
    optional_ref<assembly::BasicBlock> if_false; ///< Block to jump to if the condition is false; falls through if empty.
    bool inverse_cond = false; ///< Whether the condition polarity is inverted; `if_true`/`if_false` still refer to the same (unflipped) targets.
    bool handled = false; ///< Whether branches have already been generated for this conditional (vs. an expression value).

    /**
     * @brief Creates an independent copy of this context with a reset handled flag.
     * @return The copied context.
     */
    ConditionalContext copy() const;

    /**
     * @brief Returns a copy of this context with the condition polarity inverted.
     * @return The inverted context, with the same target blocks but flipped inverse_cond and a reset handled flag.
     */
    ConditionalContext inverse() const;

    /**
     * @brief Swaps the if_true and if_false target blocks in place.
     */
    void flip_blocks();

    /**
     * @brief Emits the conditional-branch instructions for a comparison flag already established in the given block.
     * @param block Basic block to append the branch instructions to.
     * @param flag Comparison flag to branch on (inverted first if inverse_cond is set).
     * @note No-op if `handled` is already true. Sets `handled` to true once the branches have been emitted.
     */
    void generate_branches(assembly::BasicBlock& block, constants::cmp::flag flag);

    /**
     * @brief Emits a zero-comparison against a register followed by the corresponding conditional branches.
     * @param block Basic block to append the comparison and branch instructions to.
     * @param reg Register offset holding the value to compare against zero.
     */
    void generate_branches(assembly::BasicBlock& block, uint8_t reg);

    /**
     * @brief Validates a resolved value as a boolean condition and emits the corresponding branch instructions.
     * @param ctx Compiler context providing the message list, program, and register allocator.
     * @param source Location to attribute any diagnostics to.
     * @param value Resolved guard value to test.
     * @return True on success, false if the value is not a usable boolean r-value (in which case a diagnostic is added).
     */
    bool generate_branches(Context& ctx, const message::MessageGenerator& source, const value::Value& value);
  };
}
