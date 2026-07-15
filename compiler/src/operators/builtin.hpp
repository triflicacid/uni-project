#pragma once

#include <functional>
#include "operator.hpp"
#include "memory/storage_location.hpp"

namespace lang::type {
  class Node;
}

namespace lang::ops {
  /**
   * @brief Operator implemented by the compiler itself, delegating code generation to a stored callback rather than a user-defined function body.
   */
  class BuiltinOperator : public Operator {
  public:
    using GeneratorFn = std::function<uint8_t(Context&, const std::deque<std::reference_wrapper<const value::Value>>&)>;

  protected:
    GeneratorFn generator_; ///< Callback that generates the operator's code given its arguments (left to right) and returns the result register.

  public:
    /**
     * @brief Constructs a builtin operator overload from its symbol, signature, and code-generation callback.
     * @param symbol Textual operator symbol.
     * @param type Function signature this overload matches against.
     * @param generator Callback that emits the operator's code and returns the result register.
     */
    BuiltinOperator(std::string symbol, const type::FunctionNode& type, const GeneratorFn& generator)
      : Operator(std::move(symbol), type), generator_(generator) {}

    /**
     * @brief Reports that this is a compiler-builtin operator.
     * @return Always true.
     */
    bool builtin() const override { return true; }

    /**
     * @brief Evaluates every argument to an rvalue, invokes the generator callback, and wires up the resulting register.
     * @param ctx Compiler context.
     * @param args Argument AST nodes.
     * @param return_value Output value populated with the result.
     * @param options Call-site context (unused unless a subclass overrides).
     * @return True on success, false if any argument fails to generate or is not an rvalue.
     */
    bool invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, const lang::ops::InvocationOptions &options) const override;
  };

  /**
   * @brief BuiltinOperator specialisation for comparison operators, capable of fusing directly into a conditional branch instead of materialising a boolean.
   */
  // define a built-in relational operator which is capable of conditional branching
  class RelationalBuiltinOperator : public BuiltinOperator {
    constants::cmp::flag flag_; ///< Comparison flag this operator tests when fusing into a branch.
    const type::Node& datatype_; ///< Common datatype both operands are coerced to before comparing.

  public:
    /**
     * @brief Constructs a comparison operator overload, storing both its generator callback and its branch-fusion flag/operand datatype.
     * @param symbol Textual operator symbol.
     * @param type Function signature this overload matches against.
     * @param generator Callback used in the non-fused (boolean-materialising) case.
     * @param relation Comparison flag this operator tests when fusing into a branch.
     * @param datatype Common datatype both operands are coerced to before comparing.
     */
    RelationalBuiltinOperator(std::string symbol, const type::FunctionNode& type, const BuiltinOperator::GeneratorFn& generator, constants::cmp::flag relation, const type::Node& datatype)
      : BuiltinOperator(std::move(symbol), type, generator), flag_(relation), datatype_(datatype) {}

    /**
     * @brief Generates a plain boolean comparison, or, when a conditional context is present, a fused compare-and-branch.
     * @param ctx Compiler context.
     * @param args Argument AST nodes (must be exactly two).
     * @param return_value Output value populated with the result in the non-fused case.
     * @param options Call-site context, including an optional branch-fusion target.
     * @return True on success.
     */
    bool invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, const lang::ops::InvocationOptions &options) const override;
  };

  /**
   * @brief BuiltinOperator specialisation implementing unary logical negation, capable of propagating an inverted branch condition into its operand instead of computing the negation.
   */
  // special case for handling the inverse '!' operator
  class BooleanNotBuiltinOperator : public BuiltinOperator {
  public:
    using BuiltinOperator::BuiltinOperator;

    /**
     * @brief Generates the negation, or, when a conditional context is present, delegates branching to the operand under an inverted condition.
     * @param ctx Compiler context.
     * @param args Argument AST nodes (must be exactly one).
     * @param return_value Output value populated with the result in the non-fused case.
     * @param options Call-site context, including an optional branch-fusion target.
     * @return True on success.
     */
    bool invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, const lang::ops::InvocationOptions &options) const override;
  };

  /**
   * @brief BuiltinOperator implementing short-circuiting logical && or ||.
   */
  // lazy logical && or || operator
  class LazyLogicalOperator : public BuiltinOperator {
    bool and_; ///< True for `&&`, false for `||`.

  public:
    /**
     * @brief Constructs a lazy logical operator overload.
     * @param symbol Textual operator symbol ("&&" or "||").
     * @param type Function signature this overload matches against.
     * @param is_and True for &&, false for ||.
     */
    LazyLogicalOperator(std::string symbol, const type::FunctionNode& type, bool is_and);

    /**
     * @brief Generates short-circuiting code for the operator, either fusing into the caller's branch targets or synthesizing a full result-materialising diamond.
     * @param ctx Compiler context.
     * @param args Argument AST nodes (must be exactly two).
     * @param return_value Output value populated with the result in the non-fused case.
     * @param options Call-site context, including an optional branch-fusion target and source location.
     * @return True on success.
     */
    bool invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, const lang::ops::InvocationOptions &options) const override;
  };
}
