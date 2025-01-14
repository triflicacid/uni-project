#pragma once

#include "node.hpp"
#include "symbol/symbol.hpp"

namespace lang::ast::expr {
  class SymbolReferenceNode : public Node {
    std::optional<std::reference_wrapper<symbol::Symbol>> symbol_;

  public:
    using Node::Node;

    const type::Node& type() const override { return symbol_.value().get().type(); }

    std::string name() const override { return "symbol"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    symbol::Symbol& get() { return symbol_.value(); }
  };
}
