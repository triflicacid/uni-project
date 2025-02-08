#pragma once

#include <deque>
#include "ast/node.hpp"
#include "symbol/symbol.hpp"

namespace lang::ast {
  class SymbolReferenceNode : public Node {
    std::string symbol_;

  public:
    SymbolReferenceNode(lexer::Token token, std::string symbol) : Node(std::move(token)), symbol_(std::move(symbol)) {}

    std::string node_name() const override { return "symbol"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(Context &ctx) override;

    bool resolve_lvalue(Context& ctx) override;

    bool resolve_rvalue(Context& ctx) override;
  };
}
