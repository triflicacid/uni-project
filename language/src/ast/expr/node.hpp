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
    // return may be an lvalue, or an un-loaded ("to-be") rvalue
    // in the latter case, call ::load
    virtual std::unique_ptr<value::Value> get_value(Context& ctx) const = 0;

    // guaranteed same as ::get_value(), but additionally loads value if is computable
    virtual std::unique_ptr<value::Value> load(Context& ctx) const = 0;
  };
}
