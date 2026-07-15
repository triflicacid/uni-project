#pragma once

#include "node.hpp"

namespace lang::type {
  /** @brief Marker type representing "namespace" as a pseudo-type, distinct from ast::NamespaceNode (the AST declaration node). Zero-sized; single global instance (`name_space`), letting namespace symbols fit into the normal symbol/type machinery. */
  class NamespaceNode : public Node {
  public:
    /** @brief Return the node kind name, "namespace". */
    std::string node_name() const override { return "namespace"; }

    /**
     * @brief Print "namespace" as source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Always 0: namespaces occupy no storage. */
    size_t size() const override { return 0; }

    /** @brief Return the label representation of this type, "ns". */
    std::string to_label() const override { return "ns"; }

    /** @brief Never returns: namespaces have no representable assembly datatype. */
    constants::inst::datatype::dt get_asm_datatype() const override;

    /** @brief Always false: namespaces carry no data to reference. */
    bool reference_as_ptr() const override { return false; }
  };

  extern NamespaceNode name_space;
}
