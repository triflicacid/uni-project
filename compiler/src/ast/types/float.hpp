#pragma once

#include "node.hpp"
#include "constants.hpp"

namespace lang::ast::type {
  class FloatNode : public Node {
    bool double_;

  public:
    explicit FloatNode(bool is_double) : double_(is_double) {}

    const FloatNode* get_float() const override { return this; }

    bool is_double() const { return double_; }

    std::string node_name() const override;

    std::ostream& print_code(std::ostream& os, unsigned int indent_level = 0) const override;

    size_t size() const override { return double_ ? 8 : 4; }

    bool operator==(const FloatNode& other) const {
      return double_ == other.double_;
    }

    std::string to_label() const override;

    constants::inst::datatype::dt get_asm_datatype() const override;

    bool reference_as_ptr() const override { return false; }
  };

  extern FloatNode float32, float64;
}
