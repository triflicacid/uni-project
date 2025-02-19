#pragma once

#include "value.hpp"
#include "symbol/table.hpp"
#include "ast/types/unit.hpp"

namespace lang::value {
  // represents a symbol name (reference)
  // we are not computable, either - if we were, return a Symbol
  // therefore, this is generally used when we have multiple options to pick from
  class SymbolRef : public Value {
    std::string name_;
    std::deque<std::reference_wrapper<symbol::Symbol>> overload_set_;

  public:
    SymbolRef(std::string name, std::deque<std::reference_wrapper<symbol::Symbol>> overload_set) : name_(std::move(name)), overload_set_(std::move(overload_set)) {}

    const std::string& get() const { return name_; }

    const SymbolRef* get_symbol_ref() const override { return this; }

    // return our overload set (possible symbols we may adopt)
    const std::deque<std::reference_wrapper<symbol::Symbol>>& candidates() const { return overload_set_; }

    // attempt to resolve this symbol, return value::Symbol or nullptr if error
    // argument #1 - generate messages?
    // argument #2 - type hint to help resolve overloaded symbol?
    std::unique_ptr<Symbol> resolve(const message::MessageGenerator& source, optional_ref<message::List> messages, optional_ref<const ast::type::Node> type_hint = {}) const;
  };

  // create a Value which is a symbol reference, populate overload set from ctx
  std::unique_ptr<SymbolRef> symbol_ref(const std::string name, const symbol::SymbolTable& symbols);
}
