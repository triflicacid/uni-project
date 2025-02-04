#pragma once

#include <deque>
#include "node.hpp"
#include "symbol/symbol.hpp"

namespace lang::ast::expr {
  class SymbolReferenceNode : public Node {
  public:
    using Node::Node;

    std::string node_name() const override { return "symbol"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(lang::Context &ctx) override;

    std::unique_ptr<value::Value> get_value(lang::Context &ctx) const override;

    bool resolve_lvalue(Context& ctx, value::Value & value) const override;

    bool resolve_rvalue(Context& ctx, value::Value & value) const override;
  };
}
