#pragma once

#include "operator.hpp"
#include "symbol/symbol.hpp"
#include "control-flow/conditional_context.hpp"

namespace lang::ops {
  class UserDefinedOperator : public Operator {
    const symbol::Symbol& symbol_; // tied symbol ID

  public:
    UserDefinedOperator(std::string op, const type::FunctionNode& type, const symbol::Symbol& symbol)
      : Operator(std::move(op), type), symbol_(symbol) {}

    bool builtin() const override { return false; }

    const symbol::Symbol& symbol() const { return symbol_; }

    bool invoke(lang::Context &ctx, const std::deque<std::unique_ptr<ast::Node>> &args, value::Value &return_value, const lang::ops::InvocationOptions &options) const override;
  };
}
