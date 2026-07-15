#pragma once
#include "function_base.hpp"
#include "types/function.hpp"

namespace lang::ast {
  class BlockNode;

  /**
   * @brief Concrete function definition/declaration node, distinct from type::FunctionNode (the type-system signature).
   *
   * Holds an optional body block (absent means an extern-style "declared but
   * not implemented" function) and its own local registry for the body's scope.
   */
  class FunctionNode : public FunctionBaseNode {
    std::optional<std::unique_ptr<BlockNode>> body_; ///< Function body; empty means "declared but not implemented".
    std::unique_ptr<symbol::Registry> registry_; ///< Local registry for the body's scope.

  protected:
    /**
     * @brief Process the body block, if present, within the function's scope.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool _process(Context &ctx) override;

    /**
     * @brief Generate code for the body block, if present.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool _generate_code(Context &ctx) override;

    /** @brief Return the prefix "func <name>" preceding the parameter list. */
    std::string block_prefix() const override { return "func " + name().image; }

  public:
    /**
     * @brief Construct a function node.
     * @param token Start token of the declaration.
     * @param name Token holding the function's name.
     * @param type Function signature.
     * @param params Parameter declarations.
     * @param body Body block, or empty for an unimplemented declaration.
     */
    FunctionNode(lexer::Token token, lexer::Token, const type::FunctionNode& type, std::deque<std::unique_ptr<SymbolDeclarationNode>> params, std::optional<std::unique_ptr<BlockNode>> body);

    /** @brief Return the node kind name, "function". */
    std::string node_name() const override { return "function"; }

    /**
     * @brief Print this function's signature followed by its body (or a terminating semicolon if unimplemented).
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this function and its body in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Test whether this function has a body. */
    bool is_implemented() const override { return body_.has_value(); }

    /** @brief Test whether the body, if present, unconditionally returns. */
    bool always_returns() const override;

    /**
     * @brief Register this function's symbol, then collect the body's symbols (if present) into a scoped registry.
     * @param messages Message list to report errors into.
     * @param registry Registry to create the function's symbol in.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;
  };
}