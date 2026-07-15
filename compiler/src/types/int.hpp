#pragma once

#include "node.hpp"
#include "constants.hpp"

namespace lang::type {
  /** @brief Integer type parameterized by byte width and signedness. Global instances (`uint8`, `int8`, ..., `uint64`, `int64`) are the canonical integer types. */
  class IntNode : public Node {
    bool signed_;
    uint8_t width_; // width of integer in bytes

  public:
    /**
     * @brief Construct an integer type.
     * @param width Width, in bytes.
     * @param is_signed Whether the type is signed.
     */
    IntNode(uint8_t width, bool is_signed) : signed_(is_signed), width_(width) {}

    /** @brief Return this node, since it is already an IntNode. */
    const IntNode* get_int() const override { return this; }

    /** @brief Return the node kind name, e.g. "u8", "i32". */
    std::string node_name() const override;

    /**
     * @brief Print this type's name as source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Return the width, in bytes, of this integer type. */
    size_t size() const override { return width_; }

    /**
     * @brief Order integer types by width, then unsigned before signed at equal width.
     * @param other Type to compare against.
     * @return Ordering result.
     */
    std::strong_ordering operator<=>(const IntNode& other) const {
      if (width_ == other.width_) {
        return !signed_ <=> other.signed_;
      }
      return width_ <=> other.width_;
    }

    /** @brief Return the width, in bytes, of this integer type. */
    uint8_t width() const { return width_; }

    /** @brief Test whether this integer type is signed. */
    bool is_signed() const { return signed_; }

    /** @brief Return the label representation of this type, same as `node_name`. */
    std::string to_label() const override;

    /** @brief Return the assembly datatype tag matching this type's width and signedness. */
    constants::inst::datatype::dt get_asm_datatype() const override;

    /** @brief Always false: integers are stored by value, not referenced like a pointer. */
    bool reference_as_ptr() const override { return false; }
  };

  extern IntNode uint8, int8;
  extern IntNode uint16, int16;
  extern IntNode uint32, int32;
  extern IntNode uint64, int64;
  extern IntNode uint64, int64;
}
