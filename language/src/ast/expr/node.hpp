#pragma once

#include "ast/node.hpp"
#include "ast/types/node.hpp"
#include "value/value.hpp"

namespace lang::ast::expr {
  class SymbolReferenceNode;

  class Node : public ast::Node {
  public:
    using ast::Node::Node;

    // return a Value which represents us, but does not load anything
    // call Value::is_future[r|l]value to determine which type(s) it is
    // then call ::get_rvalue or ::get_lvalue to resolve further
    virtual std::unique_ptr<value::Value> get_value(Context& ctx) const = 0;

    // given result from ::get_value(), populate the lvalue component if possible
    // returns `false` if error occurred
    virtual bool resolve_lvalue(Context& ctx, value::Value& value) const
    { return true; }

    // given result from ::get_value(), populate the rvalue component if possible
    // returns `false` if error occurred
    virtual bool resolve_rvalue(Context& ctx, value::Value& value) const
    { return true; }
  };
}
