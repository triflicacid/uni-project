#pragma once

#include "ast/node.hpp"
#include "optional_ref.hpp"

namespace lang::ast {
  /** @brief Represents `while guard { body }`, a pre-checked loop whose body must evaluate to unit. */
  class WhileStatementNode : public Node {
    unsigned int id_; ///< Unique id used to name the generated blocks.
    std::unique_ptr<Node> guard_; ///< Guard/condition expression.
    std::unique_ptr<Node> body_; ///< Loop body.

  public:
    /**
     * @brief Construct a while statement.
     * @param token Start token (`while` keyword).
     * @param guard Guard/condition expression.
     * @param body Loop body.
     */
    WhileStatementNode(lexer::Token token, std::unique_ptr<Node> guard, std::unique_ptr<Node> body);

    /** @brief Return the node kind name, "while statement". */
    std::string node_name() const override { return "while statement"; }

    /**
     * @brief Print this statement as `while guard { body }` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this statement and its guard/body in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Test whether the guard or the loop body unconditionally returns from the enclosing function. */
    bool always_returns() const override;

    /**
     * @brief Collect symbols from the guard and the loop body.
     * @param messages Message list to report errors into.
     * @param registry Registry to collect symbols into.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Process the guard (requiring a boolean-compatible type), push a loop context, process the body, and verify it evaluates to unit.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate the guard as a conditional branch, the loop body, and a branch back to the guard, followed by the trailing "after" block.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;

    /** @brief Return the label for the guard block. */
    std::string guard_label() const;

    /** @brief Return the label for the loop body's block. */
    std::string body_label() const;

    /** @brief Return the label for the block following the loop. */
    std::string end_label() const;
  };
}
