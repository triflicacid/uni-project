#pragma once

#include "value.hpp"
#include "symbol/symbol.hpp"
#include "ast/types/none.hpp"

namespace lang::value {
  // represents a symbol name (reference)
  // we are not computable, either - if we were, return a Symbol
  // therefore, this is generally used when we have multiple options to pick from
  class SymbolRef : public Value {
    std::string name_;

  public:
    explicit SymbolRef(std::string name) : Value({.computable=false}), name_(std::move(name)) {}

    const std::string& get() const { return name_; }

    const ast::type::Node& type() const override { return ast::type::none; }

    const SymbolRef* get_symbol_ref() const override { return this; }
  };

  // wrapper around symbol::Symbol
  class Symbol : public Value {
    const symbol::Symbol& symbol_;

  public:
    Symbol(const symbol::Symbol& symbol, const Options& options) : Value(options), symbol_(symbol) {}
    Symbol(const symbol::Symbol& symbol, const memory::StorageLocation& location) : Value({.lvalue=location, .computable=true}), symbol_(symbol) {}

    const symbol::Symbol& get() const { return symbol_; }

    const ast::type::Node& type() const override { return symbol_.type(); }

    const Symbol* get_symbol() const override { return this; }
  };
}
