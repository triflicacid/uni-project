#pragma once

#include "value.hpp"

namespace lang::value {
  // represents a computed, or temporary, value
  class Temporary : public Value {
    const ast::type::Node &type_;

  public:
    Temporary(const ast::type::Node& type, const Options& options) : Value(options), type_(type) {}
    Temporary(const ast::type::Node& type, const memory::Ref& ref) : Value({.rvalue=ref, .computable=false}), type_(type) {}

    const ast::type::Node& type() const override { return type_; }

    const Temporary* get_temporary() const override { return this; }
  };
}
