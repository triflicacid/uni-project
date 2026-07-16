#pragma once

#include "ast/node.hpp"
#include "optional_ref.hpp"

namespace lang::ast {
  /** @brief Represents an unconditional `loop { body }` statement, which must evaluate to unit and exits only via `break`. */
  class LoopStatementNode : public Node {
    unsigned int id_; ///< Unique id used to name the generated blocks.
    std::unique_ptr<Node> body_; ///< Loop body.

  public:
    /**
     * @brief Construct a loop statement.
     * @param token Start token (`loop` keyword).
     * @param body Loop body.
     */
    LoopStatementNode(lexer::Token token, std::unique_ptr<Node> body);

    /** @brief Return the node kind name, "loop statement". @return The string "loop statement". */
    std::string node_name() const override { return "loop statement"; }

    /**
     * @brief Print this statement as `loop { body }` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this statement and its body in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Test whether the loop body unconditionally returns from the enclosing function. @return True if the loop always returns. */
    bool always_returns() const override;

    /**
     * @brief Collect symbols from the loop body.
     * @param messages Message list to report errors into.
     * @param registry Registry to collect symbols into.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Push a loop context, process the body, and verify it evaluates to unit.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate the loop body followed by a branch back to its start, and the trailing "after" block.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;

    /** @brief Return the label for the loop body's block. @return The body block's label. */
    std::string body_label() const;

    /** @brief Return the label for the block following the loop. @return The end block's label. */
    std::string end_label() const;
  };
}
