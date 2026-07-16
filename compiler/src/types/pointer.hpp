#pragma once

#include "wrapper.hpp"

namespace lang::type {
  /** @brief Pointer-to-T type: a fixed 8-byte address. `reference_as_ptr()` is false, since a pointer is itself the stored address, not decayed data. */
  class PointerNode : public WrapperNode {
  public:
    /**
     * @brief Construct a pointer type.
     * @param inner Pointed-to type.
     */
    explicit PointerNode(const Node& inner) : WrapperNode("pointer", inner) {}

    /**
     * @brief Print this type as `*inner` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Return this node, since it is already a PointerNode. @return This node, as a PointerNode. */
    const PointerNode* get_pointer() const override { return this; }

    /** @brief Always 8: pointers are 64-bit addresses. @return Always 8. */
    size_t size() const override { return 8; }

    /** @brief Always false: a pointer is itself the stored address, not decayed reference data. @return Always false. */
    bool reference_as_ptr() const override { return false; }

    /** @brief Always the unsigned 64-bit assembly datatype. @return The unsigned 64-bit datatype tag. */
    constants::inst::datatype::dt get_asm_datatype() const override
    { return constants::inst::datatype::u64; }

    /**
     * @brief Get or create the interned pointer type for a given inner type.
     * @param inner Pointed-to type.
     * @return Reference to the matching or newly created pointer type.
     */
    static const PointerNode& get(const Node& inner);
  };
}
