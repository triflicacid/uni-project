#pragma once

#include <deque>
#include "node.hpp"
#include "symbol/symbol.hpp"

namespace lang::ast::expr {
  class SymbolReferenceNode : public Node {
  public:
    using Node::Node;

    std::string name() const override { return "symbol"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(lang::Context &ctx) override;

    std::unique_ptr<value::Value> get_value(lang::Context &ctx) const override;

    std::unique_ptr<value::Value> load(lang::Context &ctx) const override;
  };
}
