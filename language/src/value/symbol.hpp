#pragma once

#include "value.hpp"
#include "symbol/symbol.hpp"
#include "ast/types/unit.hpp"

namespace lang::value {
  // represents a symbol name (reference)
  // we are not computable, either - if we were, return a Symbol
  // therefore, this is generally used when we have multiple options to pick from
  class SymbolRef : public Value {
    std::string name_;

  public:
    explicit SymbolRef(std::string name) : Value(true, true), name_(std::move(name)) {}

    const std::string& get() const { return name_; }

    const SymbolRef* get_symbol_ref() const override { return this; }

    // attempt to resolve this symbol, return value::Symbol or nullptr if error
    // argument - generate messages?
    std::unique_ptr<Symbol> resolve(Context& ctx, const message::MessageGenerator& source, bool generate_messages) const;
  };

  // create a Value which is a symbol reference (future l+rvalue)
  std::unique_ptr<SymbolRef> symbol_ref(const std::string name);
}
