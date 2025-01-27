#pragma once

#include <map>
#include <memory>
#include <deque>
#include "symbol.hpp"
#include "assembly/basic_block.hpp"

namespace lang::symbol {
  class Variable : public Symbol {
    const ast::type::Node& type_;

  public:
    Variable(lexer::Token name, Category category, const ast::type::Node& type) : Symbol(std::move(name), std::move(category)), type_(type) {}

    bool dirty = false; // used to track if the variable contents have changed

    const ast::type::Node& type() const override { return type_; }
  };
}
