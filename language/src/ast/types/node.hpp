#pragma once

#include "ast/node.hpp"
#include "shared/constants.hpp"

namespace lang::ast::type {
  class IntNode;
  class FloatNode;
  class FunctionNode;
  class WrapperNode;
  class PointerNode;

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

    // are we a wrapper type?
    virtual const WrapperNode* get_wrapper() const { return nullptr; }

    // are we a pointer type?
    virtual const PointerNode* get_pointer() const { return nullptr; }

    // return size, in bytes, an instance of this type occupies
    virtual size_t size() const = 0;

    // return label representation of this type
    virtual std::string to_label() const = 0;

    // get asm datatype representing this node
    virtual constants::inst::datatype::dt get_asm_datatype() const = 0;

    bool operator==(const Node& other) const;
  };

  // return type node from the given asm type
  // guaranteed ::from_asm_type(t).get_asm_type() == t
  const Node& from_asm_type(constants::inst::datatype::dt type);

  // list all numerical types: uint8, int8, ..., int64, float32, float64
  extern const std::deque<std::reference_wrapper<const Node>> numerical;
}
