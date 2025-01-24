#pragma once

#include <unordered_map>
#include "symbol.hpp"
#include "assembly/basic_block.hpp"

namespace lang::symbol {
  class Registry {
    friend class SymbolTable; // allow the symbol table to access us -- it will contain us

    std::unordered_map<SymbolId, std::unique_ptr<Symbol>> symbols_;
    std::unordered_map<std::string, std::deque<SymbolId>> names_;

    // helper - insert name-id into map
    void insert_name(const Symbol& symbol);

  public:
    Registry() = default;
    Registry(const Registry&) = delete;

    bool contains(const std::string& name) const;

    const std::deque<SymbolId> get(const std::string& name) const;

    const Symbol& get(SymbolId id) const;

    Symbol& get_mut(SymbolId id);

    void remove(SymbolId id);

    void insert(std::unique_ptr<Symbol> symbol);
  };
}
