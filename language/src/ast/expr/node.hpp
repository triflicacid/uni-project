#pragma once

#include "ast/node.hpp"
#include "ast/types/node.hpp"

namespace lang::ast::expr {
class Node : public ast::Node {
  public:
    using ast::Node::Node;

    std::string name() const override { return "expression"; }

    // every expression has a type
    virtual const type::Node& type() const = 0;
  };
}
