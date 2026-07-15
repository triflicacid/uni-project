#pragma once

#include "../node.hpp"

namespace lang::ast {
  /** @brief Represents a `return [expr];` statement, unconditionally exiting the enclosing function with the given (or unit) value. */
  class ReturnNode : public Node {
    std::optional<std::unique_ptr<Node>> expr_; ///< Returned expression, if present.

  public:
    /**
     * @brief Construct a bare `return;` with no value.
     * @param token Start token (`return` keyword).
     */
    explicit ReturnNode(lexer::Token token) : Node(std::move(token)) {}

    /**
     * @brief Construct a `return expr;` statement.
     * @param token Start token (`return` keyword).
     * @param expr Returned expression, if present.
     */
    ReturnNode(lexer::Token token, std::optional<std::unique_ptr<Node>> expr) : Node(std::move(token)), expr_(std::move(expr)) {}

    /** @brief Return the node kind name, "return". */
    std::string node_name() const override { return "return"; }

    /**
     * @brief Print this statement as `return [expr]` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this statement and its optional expression in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Always true: a return statement unconditionally exits the enclosing function. */
    bool always_returns() const override { return true; }

    /** @brief Return the returned expression's value, or the unit value if none was given. */
    value::Value& value() const override;

    /**
     * @brief Collect symbols from the returned expression, if present.
     * @param messages Message list to report errors into.
     * @param registry Registry to collect symbols into.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Process the returned expression against the enclosing function's return type, erroring if used outside a function or if types mismatch.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code evaluating the returned expression (if any) and emitting the function return.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };
}
