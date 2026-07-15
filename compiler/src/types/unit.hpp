#pragma once

#include "node.hpp"

namespace lang::type {
  /** @brief The unit/void type, distinct from ast::UnitNode (the AST leaf wrapping a unit value). Zero-sized; single global instance (`unit`). */
  class UnitNode : public Node {
  public:
    /** @brief Return the node kind name, "unit". @return "unit". */
    std::string node_name() const override { return "unit"; }

    /**
     * @brief Print "()" as source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Always 0: unit values occupy no storage. @return Always 0. */
    size_t size() const override { return 0; }

    /** @brief Return the label representation of this type, "unit". @return "unit". */
    std::string to_label() const override { return "unit"; }

    /**
     * @brief Never returns: unit has no representable assembly datatype.
     * @return Never returns; throws instead.
     * @warning Always throws `std::runtime_error`. The unit type carries no assembly-level datatype, so this should never actually be called.
     */
    constants::inst::datatype::dt get_asm_datatype() const override;

    /** @brief Always false: unit values carry no data to reference. @return Always false. */
    bool reference_as_ptr() const override { return false; }
  };

  extern UnitNode unit; ///< The single global instance of the unit/void type.
}
