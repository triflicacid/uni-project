#pragma once

#include "ast/node.hpp"

namespace lang::ast::type {
  class Node : public ast::NodeBase {
  public:
    virtual bool is_int() const { return false; }

    virtual bool is_float() const { return false; }
  };
}
