#pragma once

#include <string>
#include "lexer.hpp"
#include "ast/types/node.hpp"

namespace lang::symbol {
  using SymbolId = uint32_t;

  // represents a value bound to a name
  // includes namespaces, variables, and functions (which are variables with function types)
  class Symbol {
    lexer::Token name_;
    SymbolId id_;

  public:
    explicit Symbol(lexer::Token name);

    virtual ~Symbol() = default;

    const lexer::Token& name() const { return name_; }

    uint32_t id() const { return id_; }

    virtual const ast::type::Node& type() const = 0;
  };
}
