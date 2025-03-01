#pragma once

#include "node.hpp"

namespace lang::ast::type {
  class UnitNode : public Node {
  public:
    std::string node_name() const override { return "unit"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    size_t size() const override { return 0; }

    std::string to_label() const override { return "unit"; }

    constants::inst::datatype::dt get_asm_datatype() const override;

    bool reference_as_ptr() const override { return false; }
  };

  extern UnitNode unit;
}
