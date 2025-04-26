#pragma once

#include "memory/ref.hpp"

namespace lang::type {
  class Node;
}

namespace lang::symbol {
  class Symbol;
}

namespace lang::value {
  class Symbol;
  class Reference;

  // an lvalue is something with storage
  class LValue {
    const type::Node& type_;

  public:
    explicit LValue(const type::Node& type) : type_(type) {}

    const type::Node& type() const { return type_; }

    virtual std::unique_ptr<LValue> copy() const;

    virtual const Symbol* get_symbol() const { return nullptr; }

    virtual const Reference* get_ref() const { return nullptr; }
  };

  // this lvalue refers to a symbol
  class Symbol : public LValue {
    const symbol::Symbol& symbol_;

  public:
    explicit Symbol(const symbol::Symbol& symbol);

    const Symbol* get_symbol() const override { return this; }

    const symbol::Symbol& get() const { return symbol_; }

    std::unique_ptr<LValue> copy() const override;
  };

  // this lvalue refers to a location
  class Reference : public LValue {
    memory::Ref ref_;

  public:
    Reference(const type::Node& type, memory::Ref ref) : LValue(type), ref_(std::move(ref)) {}

    const Reference* get_ref() const override { return this; }

    const memory::Ref& get() const { return ref_; }

    std::unique_ptr<LValue> copy() const override;
  };
}
