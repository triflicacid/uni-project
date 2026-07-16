#pragma once

#include "ast/node.hpp"
#include "control_flow/loop_context.hpp"

namespace lang::ast {
  /** @brief Represents `break`/`continue`, jumping to the enclosing loop's end or start label respectively. */
  class LoopControlNode : public Node {
  public:
    /** @brief Whether this node jumps to the loop's start (continue) or end (break). */
    enum class Variant {
      Break,
      Continue,
    };

    Variant variant_; ///< Whether this jumps to the loop's start (continue) or end (break).
    optional_ref<const control_flow::LoopContext> loop_; ///< Resolved enclosing loop context, populated in `::process`.

  public:
    /**
     * @brief Construct a loop-control node, inferring the variant from the token's keyword.
     * @param token `break` or `continue` token.
     */
    explicit LoopControlNode(lexer::Token token);

    /**
     * @brief Construct a loop-control node with an explicit variant.
     * @param token `break` or `continue` token.
     * @param variant Whether this jumps to the loop's start or end.
     */
    LoopControlNode(lexer::Token token, Variant variant) : Node(std::move(token)), variant_(variant) {}

    /** @brief Return the node kind name, the token's own text ("break" or "continue"). @return The node kind name. */
    std::string node_name() const override;

    /**
     * @brief Print this node as `break` or `continue` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Process this node, resolving it against the innermost enclosing loop context, erroring if none is active.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate a branch to the resolved loop's start or end label.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };
}
