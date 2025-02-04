#pragma once

namespace lang::ast::type {
  class Node;
}

namespace lang::symbol {
  class Symbol;
}

namespace lang::value {
  class Symbol;
  class Address;

  // an lvalue is something with storage
  class LValue {
    const ast::type::Node& type_;

  public:
    explicit LValue(const ast::type::Node& type) : type_(type) {}

    const ast::type::Node& type() const { return type_; }

    virtual const Symbol* get_symbol() const { return nullptr; }

    virtual const Address* get_address() const { return nullptr; }
  };

  // this lvalue refers to a symbol
  class Symbol : public LValue {
    const symbol::Symbol& symbol_;

  public:
    explicit Symbol(const symbol::Symbol& symbol);

    const Symbol* get_symbol() const override { return this; }

    const symbol::Symbol& get() const { return symbol_; }
  };

  // this lvalue refers to an address (e.g., dereferenced pointer)
  class Address : public LValue {
    uint64_t addr_;

  public:
    Address(const ast::type::Node& type, uint64_t addr) : LValue(type), addr_(addr) {}

    const Address* get_address() const override { return this; }

    uint64_t get() const { return addr_; }
  };
}
