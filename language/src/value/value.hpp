#pragma once

#include <memory>
#include <cassert>
#include "lvalue.hpp"
#include "rvalue.hpp"
#include "optional_ref.hpp"

namespace lang::ast::type {
  class Node;
}

namespace lang::value {
  class SymbolRef;

  class Value {
    std::unique_ptr<LValue> lvalue_; // store lvalue, if applicable
    std::unique_ptr<RValue> rvalue_; // store rvalue, if applicable
    std::reference_wrapper<const ast::type::Node> type_;

  public:
    Value();
    Value(const ast::type::Node& type);

    // copy ourself deeply (copies l/rvalue components)
    std::unique_ptr<Value> copy() const;

    const ast::type::Node& type() const { return type_; }

    bool is_lvalue() const { return lvalue_ != nullptr; }

    LValue& lvalue() const;

    void lvalue(std::unique_ptr<LValue> v);

    void lvalue(const symbol::Symbol& symbol);

    void lvalue(const memory::Ref& ref);

    bool is_rvalue() const { return rvalue_ != nullptr; }

    RValue& rvalue() const;

    void rvalue(std::unique_ptr<RValue> v);

    void rvalue(const memory::Ref& ref);

    virtual const SymbolRef* get_symbol_ref() const { return nullptr; }
  };

  // create a generic value
  std::unique_ptr<Value> value(optional_ref<const ast::type::Node> type = std::nullopt);

  // create an rvalue
  std::unique_ptr<Value> rvalue(const ast::type::Node& type, const memory::Ref& ref);

  // create a unit value
  std::unique_ptr<Value> unit_value();

  // value with unit type, used to signify "empty" or "none"
  extern const std::unique_ptr<Value> unit_value_instance;
}
