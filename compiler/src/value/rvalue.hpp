#pragma once

#include "memory/ref.hpp"
#include "memory/literal.hpp"

namespace lang::type {
  class Node;
}

namespace lang::value {
  // an rvalue is something which has a value
  class RValue {
    const type::Node& type_;
    memory::Ref ref_;

  public:
    RValue(const type::Node& type, const memory::Ref& ref) : type_(type), ref_(ref) {}

    virtual std::unique_ptr<RValue> copy() const;

    const type::Node& type() const { return type_; }

    const memory::Ref& ref() const { return ref_; }

    void ref(const memory::Ref& ref) { ref_ = ref; }
  };
}
