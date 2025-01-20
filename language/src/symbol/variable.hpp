#pragma once

#include <map>
#include <memory>
#include <deque>
#include "symbol.hpp"
#include "assembly/basic_block.hpp"

namespace lang::symbol {
  class Variable : public Symbol {
    const ast::type::Node& type_;
    bool used_; // are we used?

  public:
    Variable(lexer::Token name, const ast::type::Node& type) : Symbol(std::move(name)), type_(type) {}

    bool dirty = false; // used to track if the variable contents have changed

    const ast::type::Node& type() const override { return type_; }

    bool is_used() const { return used_; }

    // mark symbol as used
    void use() { used_ = true; }
  };
}
