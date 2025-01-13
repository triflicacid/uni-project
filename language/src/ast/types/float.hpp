#pragma once

#include "node.hpp"

namespace lang::ast::type {
  class FloatNode : public Node {
    bool double_;

  public:
    explicit FloatNode(bool is_double) : double_(is_double) {}

    bool is_float() const override { return true; }

    bool is_double() const { return double_; }

    std::string name() const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    bool operator==(const FloatNode& other) const {
      return double_ == other.double_;
    }
  };

  extern FloatNode float32, float64;
}
