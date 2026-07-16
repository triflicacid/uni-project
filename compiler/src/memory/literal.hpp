#pragma once

#include <cstdint>
#include <string>
#include "constants.hpp"

namespace lang::type {
  class Node;
  class ArrayNode;
}

namespace lang::memory {
  /**
   * @brief An interned constant value: a single 64-bit word of raw data paired with its type.
   *
   * Instances are uniqued by (type, data) and obtained only through the static
   * factory methods; the constructor is not publicly accessible.
   */
  class Literal {
    const type::Node& type_; ///< This literal's type.
    uint64_t data_; ///< Raw 64-bit data word.

    Literal(const Literal&) = delete;

  protected:
    /**
     * @brief Constructs a literal from its type and raw data; use @ref get to obtain an interned instance instead of calling this directly.
     * @param type The literal's type.
     * @param data Raw 64-bit data word.
     */
    Literal(const type::Node& type, uint64_t data) : type_(type), data_(data) {}

  public:
    /**
     * @brief Returns the literal's type.
     * @return The type.
     */
    const type::Node& type() const { return type_; }

    /**
     * @brief Returns the literal's raw 64-bit data.
     * @return The raw data word.
     */
    uint64_t data() const { return data_; }

    /**
     * @brief Renders the numeric literal in the string form appropriate for its type.
     * @return The string representation.
     */
    std::string to_string() const;

    /**
     * @brief Reinterprets this literal's data as another datatype, converting the raw bit pattern accordingly.
     * @param target Type to convert to.
     * @return The interned literal of the target type holding the converted data.
     */
    const Literal& change_type(const type::Node& target) const;

    /**
     * @brief Returns the interned literal for a given (type, data) pair, creating it if it does not already exist.
     * @param type Type of the literal.
     * @param data Raw 64-bit data of the literal.
     * @return The interned literal.
     */
    static const Literal& get(const type::Node& type, uint64_t data);

    /**
     * @brief Returns the interned zero-valued literal of a given type.
     * @param type Type of the literal.
     * @return The interned zero literal.
     */
    static const Literal& zero(const type::Node& type);

    /**
     * @brief Returns the interned boolean literal for a given value.
     * @param b Boolean value to represent.
     * @return The interned boolean literal.
     */
    static const Literal& get_boolean(bool b);

    /**
     * @brief Compares two literals for equality by type and raw data.
     * @param other Literal to compare against.
     * @return True if equal; arrays are never considered equal.
     */
    bool operator==(const Literal& other) const;
  };
}