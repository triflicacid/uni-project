#pragma once

#include "ast/node.hpp"
#include "memory/literal.hpp"

namespace lang::ast {
  // wrapper for `()` unit
  // essentially, a node wrapper for `value::unit_value`
  class UnitNode : public Node {
  public:
    using Node::Node;

    std::string node_name() const override { return "unit"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(Context &ctx) override;

    value::Value& value() const override;
  };
}
