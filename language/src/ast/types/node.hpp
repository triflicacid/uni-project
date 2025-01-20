#pragma once

#include "ast/node.hpp"

namespace lang::ast::type {
  class IntNode;
  class FloatNode;

  class Node : public ast::NodeBase {
  public:
    // return this as int if we are an int, else return nullptr
    virtual const IntNode* get_int() const { return nullptr; }

    // return this as float if we are a float, else return nullptr
    virtual const FloatNode* get_float() const { return nullptr; }

    // return size, in bytes, an instance of this type occupies
    virtual size_t size() const = 0;
  };
}
