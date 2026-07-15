#pragma once

#include "wrapper.hpp"

namespace lang::type {
  class PointerNode;

  /** @brief Fixed-size array type `[T; N]`. `reference_as_ptr()` is true, since arrays decay to a pointer when referenced. */
  class ArrayNode : public WrapperNode {
    size_t size_; ///< Number of elements.

  public:
    /**
     * @brief Construct an array type.
     * @param inner Element type.
     * @param size Number of elements.
     */
    ArrayNode(const Node& inner, size_t size);

    /** @brief Return this node, since it is already an ArrayNode. */
    const ArrayNode* get_array() const override { return this; }

    /** @brief Always true: arrays decay to a pointer to their first element when referenced. */
    bool reference_as_ptr() const override { return true; }

    /**
     * @brief Return the type of the given property; supports "length" (a `uint64`).
     * @param property Property name.
     * @return The property's type, or nothing if no such property exists.
     */
    optional_ref<const Node> get_property_type(const std::string &property) const override;

    /**
     * @brief Get the value of the given property; supports "length" (the array's element count as a literal).
     * @param ctx Compilation context.
     * @param property Property name.
     * @return The property's value, or nullptr on error.
     */
    std::unique_ptr<value::Value> get_property(Context &ctx, const std::string &property) const override;

    /** @brief Return the total size in bytes, i.e. the element type's size times the element count. */
    size_t size() const override;

    /**
     * @brief Print this type as `[inner; size]` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Always the unsigned 64-bit assembly datatype (arrays are handled as pointers). */
    constants::inst::datatype::dt get_asm_datatype() const override
    { return constants::inst::datatype::u64; } // pointer type

    /** @brief Return the pointer type this array decays into when referenced. */
    const PointerNode& decay_into_pointer() const;

    /**
     * @brief Get or create the interned array type for a given element type and length.
     * @param inner Element type.
     * @param size Number of elements.
     * @return Reference to the matching or newly created array type.
     */
    static const ArrayNode& get(const Node& inner, size_t size);
  };
}
