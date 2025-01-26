#pragma once

#include <deque>
#include "node.hpp"
#include "symbol/symbol.hpp"

namespace lang::ast::expr {
  class SymbolReferenceNode : public Node {
    std::optional<std::reference_wrapper<symbol::Symbol>> symbol_;

  protected:
    // this function is used to generate a suitable candidate
    // guaranteed |candidates|>0
    // for a basic variable reference, return first candidate
    virtual std::optional<std::reference_wrapper<symbol::Symbol>> select_candidate(Context& ctx, const std::deque<std::reference_wrapper<symbol::Symbol>>& candidates) const;

  public:
    using Node::Node;

    const type::Node& type() const override { return symbol_.value().get().type(); }

    std::string name() const override { return "symbol"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    symbol::Symbol& get() { return symbol_.value(); }

    bool process(lang::Context &ctx) override;

    bool load(lang::Context &ctx) const override;
  };
}
