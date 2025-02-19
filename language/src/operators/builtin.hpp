#pragma once

#include <functional>
#include "operator.hpp"

namespace lang::ast::type {
  class Node;
}

namespace lang::ops {
  class BuiltinOperator : public Operator {
  public:
    using GeneratorFn = std::function<uint8_t(Context&, const std::deque<std::reference_wrapper<const value::Value>>&)>;

  protected:
    // function to generate appropriate code
    // provided array of arguments -- left to right
    // returns result
    GeneratorFn generator_;

  public:
    BuiltinOperator(std::string symbol, const ast::type::FunctionNode& type, const GeneratorFn& generator)
      : Operator(std::move(symbol), type), generator_(generator) {}

    bool builtin() const override { return true; }

    bool invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, optional_ref<ast::ConditionalContext> conditional = std::nullopt) const override;
  };

  // define a built-in relational operator which is capable of conditional branching
  class RelationalBuiltinOperator : public BuiltinOperator {
    constants::cmp::flag flag_;
    const ast::type::Node& datatype_;

  public:
    RelationalBuiltinOperator(std::string symbol, const ast::type::FunctionNode& type, const BuiltinOperator::GeneratorFn& generator, constants::cmp::flag relation, const ast::type::Node& datatype)
      : BuiltinOperator(std::move(symbol), type, generator), flag_(relation), datatype_(datatype) {}

    bool invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, optional_ref<ast::ConditionalContext> conditional = std::nullopt) const override;
  };

  // special case for handling the inverse '!' operator
  class BooleanNotBuiltinOperator : public BuiltinOperator {
  public:
    using BuiltinOperator::BuiltinOperator;

    bool invoke(Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, optional_ref<ast::ConditionalContext> conditional = std::nullopt) const override;
  };
}
