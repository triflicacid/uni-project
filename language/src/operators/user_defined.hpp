#pragma once

#include "operator.hpp"
#include "symbol/symbol.hpp"

namespace lang::ops {
  class UserDefinedOperator : public Operator {
    const symbol::Symbol& symbol_; // tied symbol ID

  public:
    UserDefinedOperator(std::string op, const ast::type::FunctionNode& type, const symbol::Symbol& symbol)
      : Operator(std::move(op), type), symbol_(symbol) {}

    bool builtin() const override { return false; }

    const symbol::Symbol& symbol() const { return symbol_; }
  };
}
