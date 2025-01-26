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

    // ::process just processes the symbol, i.e., prepares it for use
    // call this to load into the register allocation
    virtual bool load(Context& ctx) const
    { throw std::runtime_error(name() + "::load is unimplemented"); }
  };
}
