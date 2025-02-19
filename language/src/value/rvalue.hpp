#pragma once

#include "memory/ref.hpp"
#include "memory/literal.hpp"

namespace lang::ast::type {
  class Node;
}

namespace lang::value {
  class Literal;

  // an rvalue is something which has a value
  class RValue {
    const ast::type::Node& type_;
    memory::Ref ref_;

  public:
    RValue(const ast::type::Node& type, const memory::Ref& ref) : type_(type), ref_(ref) {}

    virtual std::unique_ptr<RValue> copy() const;

    const ast::type::Node& type() const { return type_; }

    const memory::Ref& ref() const { return ref_; }

    void ref(const memory::Ref& ref) { ref_ = ref; }

    virtual const Literal* get_literal() const { return nullptr; }
  };

  // an rvalue for a literal, essentially a wrapper around memory::Literal
  class Literal : public RValue {
    const memory::Literal& lit_;

  public:
    explicit Literal(const memory::Literal& lit, const memory::Ref& ref) : RValue(lit.type(), ref), lit_(lit) {}

    std::unique_ptr<RValue> copy() const override;

    const memory::Literal& get() const { return lit_; }

    const Literal* get_literal() const override { return this; }
  };
}
