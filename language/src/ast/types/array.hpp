#pragma once

#include "wrapper.hpp"

namespace lang::ast::type {
  class PointerNode;

  class ArrayNode : public WrapperNode {
    size_t size_;

  public:
    ArrayNode(const ast::type::Node& inner, size_t size);
    explicit ArrayNode(const ast::type::Node& inner);

    const ArrayNode* get_array() const override { return this; }

    bool reference_as_ptr() const override { return true; }

    optional_ref<const Node> get_property_type(const std::string &property) const override;

    bool get_property(Context &ctx, value::Value& result, const std::string &property) const override;

    // this return the size of the array, i.e., unwrap.size() * size_
    size_t size() const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    constants::inst::datatype::dt get_asm_datatype() const override
    { return constants::inst::datatype::u64; } // pointer type

    const PointerNode& decay_into_pointer() const;

    // create or return existing reference
    static const ArrayNode& get(const lang::ast::type::Node& inner, size_t size);
  };
}
