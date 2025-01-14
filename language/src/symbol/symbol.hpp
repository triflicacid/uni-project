#pragma once

#include <string>
#include "lexer.hpp"
#include "ast/types/node.hpp"

namespace lang::symbol {
  class Symbol {
    lexer::Token name_;

  public:
    explicit Symbol(lexer::Token name) : name_(std::move(name)) {}

    const lexer::Token& name() const { return name_; }

    virtual const ast::type::Node& type() const = 0;
  };
}
