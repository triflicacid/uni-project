#pragma once

#include <string>
#include "ast/types/node.hpp"

namespace lang::symbol {
  using SymbolId = uint32_t;

  // represents a value bound to a name
  // includes namespaces, variables, and functions (which are variables with function types)
  class Symbol {
    lexer::Token token_; // origin token (top-level name)
    std::optional<std::reference_wrapper<Symbol>> parent_; // parent symbol (i.e., namespace)
    SymbolId id_;

  public:
    explicit Symbol(lexer::Token name);

    virtual ~Symbol() = default;

    const lexer::Token& token() const { return token_; }

    const std::string& name() const { return token_.image; }

    // generate fully-qualified name (by tracking parents)
    std::string full_name() const;

    const std::optional<std::reference_wrapper<Symbol>>& parent() const { return parent_; }

    uint32_t id() const { return id_; }

    virtual const ast::type::Node& type() const = 0;
  };
}
