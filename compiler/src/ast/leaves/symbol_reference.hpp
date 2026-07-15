#pragma once

#include <deque>
#include "ast/node.hpp"
#include "symbol/symbol.hpp"

namespace lang::ast {
  /** @brief Leaf node representing a bare identifier reference, resolved (phase 3) against possibly multiple candidate symbols sharing that name. */
  class SymbolReferenceNode : public Node {
    std::string symbol_; ///< Referenced symbol name.

  public:
    /**
     * @brief Construct a symbol reference.
     * @param token Token holding the referenced name.
     * @param symbol Referenced symbol name.
     */
    SymbolReferenceNode(lexer::Token token, std::string symbol) : Node(std::move(token)), symbol_(std::move(symbol)) {}

    /** @brief Return the node kind name, "symbol". */
    std::string node_name() const override { return "symbol"; }

    /**
     * @brief Print this reference as its bare identifier text.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this reference's name in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Build a pending symbol reference for this name, rejecting the discard symbol `_`.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Resolve the pending symbol reference to a concrete symbol.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool resolve(Context & ctx) override;

    /**
     * @brief Ensure the resolved symbol, if it names a value, is defined.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(lang::Context &ctx) override;
  };
}
