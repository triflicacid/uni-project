#pragma once

#include "wrapper.hpp"

namespace lang::type {
  class PointerNode;

  class ArrayNode : public WrapperNode {
    size_t size_;

  public:
    ArrayNode(const Node& inner, size_t size);

    const ArrayNode* get_array() const override { return this; }

    bool reference_as_ptr() const override { return true; }

    optional_ref<const Node> get_property_type(const std::string &property) const override;

    std::unique_ptr<value::Value> get_property(Context &ctx, const std::string &property) const override;

    // this return the size of the array, i.e., unwrap.size() * size_
    size_t size() const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    constants::inst::datatype::dt get_asm_datatype() const override
    { return constants::inst::datatype::u64; } // pointer type

    const PointerNode& decay_into_pointer() const;

    // create or return existing reference
    static const ArrayNode& get(const Node& inner, size_t size);
  };
}
