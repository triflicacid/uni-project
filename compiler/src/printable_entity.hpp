#pragma once

#include <string>

namespace lang {
  /**
   * @brief Abstract base giving any node a uniform way to report its kind and render itself as source-like code.
   *
   * Common ancestor of the unrelated ast::Node and type::Node hierarchies.
   */
  class PrintableEntity {
  public:
    virtual ~PrintableEntity() = default;

    /**
     * @brief Returns a short human-readable name identifying the concrete node kind.
     * @return Name of the node kind (e.g. "BlockNode").
     */
    virtual std::string node_name() const = 0;

    /**
     * @brief Writes the node's source-code representation to a stream.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth, in indentation units.
     * @return The same stream, for chaining.
     */
    virtual std::ostream& print_code(std::ostream& os, unsigned int indent_level = 0) const = 0;
  };
}
