#pragma once

#include "ast/node.hpp"
#include "optional_ref.hpp"

namespace lang::ast {
  /**
   * @brief Represents `if guard then_body [else else_body]`.
   *
   * Stores the guard, then-branch, and optional else-branch as child nodes,
   * plus a unique id used to name the generated branch/label blocks.
   */
  class IfStatementNode : public Node {
    unsigned int id_; ///< Unique id used to name the generated branch/label blocks.
    std::unique_ptr<Node> guard_; ///< Guard/condition expression.
    std::unique_ptr<Node> then_; ///< Body executed when the guard is true.
    std::optional<std::unique_ptr<Node>> else_; ///< Body executed when the guard is false, if present.
    std::optional<lexer::Token> else_token_; ///< Token of the `else` keyword, if present.

  public:
    /**
     * @brief Construct an if statement.
     * @param token Start token (`if` keyword).
     * @param guard Guard/condition expression.
     * @param then_body Body executed when the guard is true.
     * @param else_token Token of the `else` keyword, if present.
     * @param else_body Body executed when the guard is false, if present.
     */
    IfStatementNode(lexer::Token token, std::unique_ptr<Node> guard, std::unique_ptr<Node> then_body, std::optional<lexer::Token> else_token, std::optional<std::unique_ptr<Node>> else_body);

    /** @brief Return the node kind name, "if statement". @return The string "if statement". */
    std::string node_name() const override { return "if statement"; }

    /**
     * @brief Print this statement as `if guard then_body [else else_body]` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this statement and its guard/branches in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Test whether the guard, or both branches (else defaulting to not returning if absent), unconditionally return. @return True if the statement always returns. */
    bool always_returns() const override;

    /** @brief Always true: an if statement may write its result to `$ret`. @return True. */
    bool writes_to_ret() const override;

    /**
     * @brief Collect symbols from the guard and both branches.
     * @param messages Message list to report errors into.
     * @param registry Registry to collect symbols into.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Process the guard and both branches, checking that non-returning branch types agree.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate branch code for the guard, then the then/else branches, joining their results into a single register if needed.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };
}
