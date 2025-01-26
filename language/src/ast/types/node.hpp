#pragma once

#include "ast/node.hpp"

namespace lang::ast::type {
  class IntNode;
  class FloatNode;
  class FunctionNode;

  using TypeId = unsigned int;

  class Node : public ast::NodeBase {
    TypeId id_;

  public:
    Node();

    TypeId id() const { return id_; }

    // return this as int if we are an int, else return nullptr
    virtual const IntNode* get_int() const { return nullptr; }

    // return this as float if we are a float, else return nullptr
    virtual const FloatNode* get_float() const { return nullptr; }

    // return this as function is we are a function, else return nullptr
    virtual const FunctionNode* get_func() const { return nullptr; }

    // return size, in bytes, an instance of this type occupies
    virtual size_t size() const = 0;

    // return label representation of this type
    virtual std::string to_label() const = 0;
  };
}
