#pragma once

#include "ast/node.hpp"
#include "memory/literal.hpp"

namespace lang::ast {
  /** @brief AST leaf wrapping a scalar memory::Literal, with an optional type suffix overriding type-hint-based inference. */
  class LiteralNode : public Node {
    optional_ref<const memory::Literal> lit_; ///< Resolved literal value, set once `process` has run.
    optional_ref<const type::Node> suffix_; ///< Explicit type suffix, if set; overrides type-hint-based inference.

    /** @brief Determine the numeric type this literal should be cast to, based on any type suffix/hint or the literal's own float/int form. @return The target numeric type. */
    const type::Node& get_target_numeric_type() const;

  public:
    /**
     * @brief Construct a literal node whose value has not yet been resolved.
     * @param token Token holding the literal's source text.
     */
    LiteralNode(lexer::Token token) : Node(std::move(token)) {}

    /**
     * @brief Construct a literal node wrapping an already-resolved literal value.
     * @param token Token holding the literal's source text.
     * @param lit Resolved literal value.
     */
    LiteralNode(lexer::Token token, const memory::Literal& lit) : Node(std::move(token)), lit_(lit) {}

    /**
     * @brief Set an explicit type suffix, overriding type-hint-based inference.
     * @param type Suffix type.
     */
    void suffix(const type::Node& type) { suffix_ = type; }

    /** @brief Return the node kind name, "literal". @return The string "literal". */
    std::string node_name() const override { return "literal"; }

    /** @brief Return the resolved literal value. Assumes `process` has run. @return The resolved literal value. */
    const memory::Literal& get() const;

    /**
     * @brief Print this literal's original source text.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this literal's text and resolved type in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Resolve this literal's numeric/pointer/boolean value, parsing the source text against the target type.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;
  };
}
