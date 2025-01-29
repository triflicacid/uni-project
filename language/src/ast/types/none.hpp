#pragma once

#include "node.hpp"

namespace lang::ast::type {
  class NoneNode : public Node {
  public:
    std::string name() const override { return "none"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    size_t size() const override { return 0; }

    std::string to_label() const override { return "none"; }

    constants::inst::datatype::dt get_asm_datatype() const override;
  };

  extern NoneNode none;
}
