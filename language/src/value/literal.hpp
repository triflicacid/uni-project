#pragma once

#include "value.hpp"
#include "memory/literal.hpp"

namespace lang::value {
  // wrapper around memory::Literal
  class Literal : public Value {
    const memory::Literal& lit_;

  public:
    explicit Literal(const memory::Literal& lit) : Value({.computable=true}), lit_(lit) {}
    Literal(const memory::Literal& lit, const Options options) : Value(options), lit_(lit) {}

    const memory::Literal& get() const { return lit_; }

    const ast::type::Node& type() const override { return lit_.type(); }

    const Literal* get_literal() const override { return this; }
  };
}
