#pragma once

#include <map>
#include <memory>
#include <deque>
#include "symbol.hpp"
#include "lexer.hpp"
#include "ast/types/namespace.hpp"

namespace lang::symbol {
  class Namespace : public Symbol {
    std::map<std::string, std::deque<std::unique_ptr<Symbol>>> members_; // contains deque due to function overloading

  public:
    explicit Namespace(lexer::Token name) : Symbol(std::move(name)) {}

    const ast::type::Node& type() const override { return ast::type::name_space; }

    // get all symbols with this name, or nullptr
    const std::deque<std::unique_ptr<Symbol>>* find(const std::string& name) const;

    // insert new symbol into map
    // note: does no checks, just inserts it
    void insert(std::unique_ptr<Symbol> symbol);
  };
}
