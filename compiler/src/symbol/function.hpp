#pragma once

#include "symbol.hpp"

namespace lang::ast {
  class FunctionBaseNode;
}

namespace lang::symbol {
  /**
   * @brief Symbol subclass representing a function or operator binding, overriding define() to trigger the originating AST node's code generation on demand.
   */
  class Function : public Symbol {
    ast::FunctionBaseNode& node_;

  public:
    /**
     * @brief Constructs a function symbol bound to its originating AST definition node.
     * @param name Origin token providing the function's name.
     * @param node AST node defining the function's signature and body.
     */
    Function(lexer::Token name, ast::FunctionBaseNode& node);

    /**
     * @brief Returns the AST node that defines this function.
     * @return The originating function-definition node.
     */
    ast::FunctionBaseNode& origin() const { return node_; }

    /**
     * @brief Triggers the function's code generation on first use, forwarding to the originating node.
     * @param ctx Compiler context.
     * @return True on success.
     */
    bool define(lang::Context &ctx) const override;
  };
}
