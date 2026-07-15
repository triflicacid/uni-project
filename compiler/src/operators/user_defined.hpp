#pragma once

#include "operator.hpp"
#include "symbol/symbol.hpp"
#include "control_flow/conditional_context.hpp"

namespace lang::ops {
  /**
   * @brief Operator implemented by a user-written operator function, invoked as an ordinary function call rather than inline codegen.
   */
  class UserDefinedOperator : public Operator {
    const symbol::Symbol& symbol_; ///< Function symbol implementing this operator.

  public:
    /**
     * @brief Constructs a user-defined operator overload bound to the function symbol implementing it.
     * @param op Textual operator symbol.
     * @param type Function signature this overload matches against.
     * @param symbol Function symbol implementing the operator.
     */
    UserDefinedOperator(std::string op, const type::FunctionNode& type, const symbol::Symbol& symbol)
      : Operator(std::move(op), type), symbol_(symbol) {}

    /**
     * @brief Reports that this is a user-defined (not builtin) operator.
     * @return Always false.
     */
    bool builtin() const override { return false; }

    /**
     * @brief Returns the function symbol implementing this operator.
     * @return The backing symbol.
     */
    const symbol::Symbol& symbol() const { return symbol_; }

    /**
     * @brief Forces lazy definition of the backing function and generates a call to it.
     * @param ctx Compiler context.
     * @param args Argument AST nodes.
     * @param return_value Output value populated with the call's result.
     * @param options Call-site context (ignored; user-defined operators never fuse into branches).
     * @return True on success, false if the backing function fails to define.
     */
    bool invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, const lang::ops::InvocationOptions &options) const override;
  };
}
