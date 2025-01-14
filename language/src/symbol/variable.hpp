#pragma once

#include <map>
#include <memory>
#include <deque>
#include "symbol.hpp"

namespace lang::symbol {
  class Variable : public Symbol {
    const ast::type::Node& type_;

  public:
    Variable(lexer::Token name, const ast::type::Node& type) : Symbol(std::move(name)), type_(type) {}

    const ast::type::Node& type() const override { return type_; }

    const ast::type::Node& datatype() const { return type_; }
  };
}
