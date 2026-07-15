#pragma once

#include "node.hpp"
#include "constants.hpp"

namespace lang::type {
  /** @brief Floating-point type parameterized by width (32-bit `float` vs. 64-bit `double`). Global instances `float32`/`float64` are the canonical float types. */
  class FloatNode : public Node {
    bool double_; ///< Whether this is the 64-bit double-precision type (false for 32-bit).

  public:
    /**
     * @brief Construct a floating-point type.
     * @param is_double Whether this is the 64-bit double-precision type (false for 32-bit).
     */
    explicit FloatNode(bool is_double) : double_(is_double) {}

    /** @brief Return this node, since it is already a FloatNode. @return This node, as a FloatNode. */
    const FloatNode* get_float() const override { return this; }

    /** @brief Test whether this is the 64-bit double-precision type. @return True if this is the 64-bit double-precision type. */
    bool is_double() const { return double_; }

    /** @brief Return the node kind name, "f32" or "f64". @return "f32" or "f64". */
    std::string node_name() const override;

    /**
     * @brief Print this type's name as source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream& os, unsigned int indent_level = 0) const override;

    /** @brief Return 8 for double-precision, 4 for single-precision. @return The size in bytes: 8 or 4. */
    size_t size() const override { return double_ ? 8 : 4; }

    /**
     * @brief Compare float types by width.
     * @param other Type to compare against.
     * @return True if both have the same precision.
     */
    bool operator==(const FloatNode& other) const {
      return double_ == other.double_;
    }

    /** @brief Return the label representation of this type, same as `node_name`. @return The label, same as `node_name()`. */
    std::string to_label() const override;

    /** @brief Return the assembly datatype tag matching this type's precision. @return The datatype tag matching this type's precision. */
    constants::inst::datatype::dt get_asm_datatype() const override;

    /** @brief Always false: floats are stored by value, not referenced like a pointer. @return Always false. */
    bool reference_as_ptr() const override { return false; }
  };

  extern FloatNode float32; ///< The single global instance of the 32-bit single-precision float type.
  extern FloatNode float64; ///< The single global instance of the 64-bit double-precision float type.
}
