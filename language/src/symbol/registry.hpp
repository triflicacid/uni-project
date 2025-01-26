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

    size_t size() const { return symbols_.size(); }

    bool empty() const { return symbols_.empty(); }

    bool contains(const std::string& name) const;

    const std::deque<SymbolId> get(const std::string& name) const;

    const Symbol& get(SymbolId id) const;

    void remove(SymbolId id);

    void insert(std::unique_ptr<Symbol> symbol);

    auto begin() { return symbols_.begin(); }
    auto begin() const { return symbols_.begin(); }
    auto end() { return symbols_.end(); }
    auto end() const { return symbols_.end(); }
  };

  // create and insert a variable into the given registry if permitted
  // return symbol's id, or nothing if error
  std::optional<SymbolId> create_variable(lang::symbol::Registry& registry, const Category& category, const lang::lexer::Token& token, const lang::ast::type::Node& type, message::List& messages);
}
