#pragma once

#include "symbol.hpp"

namespace lang::ast {
  class FunctionBaseNode;
}

namespace lang::symbol {
  class Function : public Symbol {
    ast::FunctionBaseNode& node_;

  public:
    Function(lexer::Token name, ast::FunctionBaseNode& node);

    ast::FunctionBaseNode& origin() const { return node_; }

    bool define(lang::Context &ctx) const override;
  };
}
