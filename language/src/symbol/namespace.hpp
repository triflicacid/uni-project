#pragma once

#include <map>
#include <memory>
#include <unordered_set>
#include "symbol.hpp"
#include "ast/types/namespace.hpp"

namespace lang::symbol {
  class Namespace : public Symbol {
  public:
    explicit Namespace(lexer::Token name) : Symbol(std::move(name), Category::Namespace) {}

    const ast::type::Node& type() const override { return ast::type::name_space; }
  };
}
