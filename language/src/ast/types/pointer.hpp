#pragma once

#include "wrapper.hpp"

namespace lang::ast::type {
  class PointerNode : public WrapperNode {
  public:
    explicit PointerNode(const Node& inner) : WrapperNode("pointer", inner) {}

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    const PointerNode* get_pointer() const override { return this; }

    size_t size() const override { return 8; }

    bool reference_as_ptr() const override { return false; }

    constants::inst::datatype::dt get_asm_datatype() const override
    { return constants::inst::datatype::u64; }

    // create or return existing reference
    static const PointerNode& get(const lang::ast::type::Node& inner);
  };
}
