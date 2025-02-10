#pragma once

#include <memory>
#include <cassert>
#include "lvalue.hpp"
#include "rvalue.hpp"

namespace lang::ast::type {
  class Node;
}

namespace lang::value {
  class SymbolRef;

  class Value {
    std::unique_ptr<LValue> lvalue_; // store lvalue, if applicable
    bool future_lvalue_; // indicates that we have the ability to become an lvalue (after computation)

    std::unique_ptr<RValue> rvalue_;
    bool future_rvalue_;

    std::reference_wrapper<const ast::type::Node> type_;

  public:
    explicit Value();
    Value(bool future_lvalue, bool future_rvalue);
    Value(const ast::type::Node& type, bool future_lvalue, bool future_rvalue);

    const ast::type::Node& type() const { return type_; }

    bool is_future_lvalue() const { return future_lvalue_; }

    bool is_lvalue() const { return lvalue_ != nullptr; }

    LValue& lvalue() const;

    void lvalue(std::unique_ptr<LValue> v);

    void lvalue(const memory::Ref& ref);

    bool is_future_rvalue() const { return future_rvalue_; }

    bool is_rvalue() const { return rvalue_ != nullptr; }

    RValue& rvalue() const;

    void rvalue(std::unique_ptr<RValue> v);

    void rvalue(const memory::Ref& ref);

    virtual const SymbolRef* get_symbol_ref() const { return nullptr; }
  };

  // create a Value which is a future rvalue
  std::unique_ptr<Value> rvalue();
  std::unique_ptr<Value> rvalue(const ast::type::Node& type);

  // create a Value which is a future lvalue
  std::unique_ptr<Value> lvalue();
  std::unique_ptr<Value> lvalue(const ast::type::Node& type);

  // create a Value which is a future l-value & rvalue
  std::unique_ptr<Value> rlvalue();
  std::unique_ptr<Value> rlvalue(const ast::type::Node& type);

  // value with unit type, cannot be anything els
  // used to signify "empty" or "none"
  extern const Value unit_value;
}
