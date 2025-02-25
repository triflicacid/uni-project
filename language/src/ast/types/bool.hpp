#pragma once

#include "node.hpp"
#include "constants.hpp"

namespace lang::ast::type {
  class BoolNode : public Node {
  public:
    BoolNode() = default;

    std::string node_name() const override { return "bool"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    size_t size() const override { return 1; }

    std::string to_label() const override { return "bool"; }

    constants::inst::datatype::dt get_asm_datatype() const override;

    bool reference_as_ptr() const override { return false; }
  };

  extern BoolNode boolean;
}
