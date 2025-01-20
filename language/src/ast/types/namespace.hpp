#pragma once

#include "node.hpp"

namespace lang::ast::type {
  class NamespaceNode : public Node {
  public:
    std::string name() const override { return "type::namespace"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    size_t size() const override { return 0; }
  };

  extern NamespaceNode name_space;
}
