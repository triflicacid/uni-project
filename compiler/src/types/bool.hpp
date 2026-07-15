#pragma once

#include "node.hpp"
#include "constants.hpp"

namespace lang::type {
  /** @brief The boolean type. Trivial, fixed 1-byte size; a single global instance (`boolean`) is used everywhere a boolean type is needed. */
  class BoolNode : public Node {
  public:
    BoolNode() = default;

    /** @brief Return the node kind name, "bool". */
    std::string node_name() const override { return "bool"; }

    /**
     * @brief Print "bool" as source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Always 1: booleans occupy one byte. */
    size_t size() const override { return 1; }

    /** @brief Return the label representation of this type, "bool". */
    std::string to_label() const override { return "bool"; }

    /** @brief Return the assembly datatype tag used for booleans (unsigned 32-bit). */
    constants::inst::datatype::dt get_asm_datatype() const override;

    /** @brief Always false: booleans are stored by value, not referenced like a pointer. */
    bool reference_as_ptr() const override { return false; }
  };

  extern BoolNode boolean; ///< The single global instance of the boolean type.
}
