#pragma once

#include "operator.hpp"
#include "symbol/symbol.hpp"

namespace lang::ops {
  class UserDefinedOperator : public Operator {
    symbol::SymbolId symbol_; // tied symbol ID

  public:
    UserDefinedOperator(std::string symbol, const ast::type::FunctionNode& type, symbol::SymbolId symbol_id)
      : Operator(std::move(symbol), type), symbol_(symbol_id) {}

    bool builtin() const override { return false; }

    symbol::SymbolId symbol_id() const { return symbol_; }
  };
}
