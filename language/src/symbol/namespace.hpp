#pragma once

#include <map>
#include <memory>
#include <unordered_set>
#include "symbol.hpp"
#include "ast/types/namespace.hpp"

namespace lang::symbol {
  class Namespace : public Symbol {
  public:
    using Symbol::Symbol;

    const ast::type::Node& type() const override { return ast::type::name_space; }
  };
}
