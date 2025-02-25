#pragma once

#include "ast/node.hpp"
#include "shared/constants.hpp"

namespace lang::ast::type {
  class IntNode;
  class FloatNode;
  class FunctionNode;
  class WrapperNode;
  class PointerNode;
  class ArrayNode;

  using TypeId = unsigned int;

  class Node : public ast::NodeBase {
    TypeId id_;

  public:
    Node();
    Node(const Node&) = delete;

    TypeId id() const { return id_; }

    // get this property's return type, or none if property does not exist
    virtual optional_ref<const Node> get_property_type(const std::string& property) const;

    // get the given property, assume property exists by calling `::get_property_type`, return success
    virtual bool get_property(Context& ctx, value::Value& result, const std::string& property) const;

    // do we act as a pointer: point to first element of structure, but treated as ptr
    // the difference is a pointer is an actual stored address, whereas something with this as `true` is just the data
    // when assigned, such values are copied
    virtual bool reference_as_ptr() const = 0;

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

    // are we an array type?
    virtual const ArrayNode* get_array() const { return nullptr; }

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
