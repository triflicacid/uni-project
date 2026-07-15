#pragma once

#include "ast/node.hpp"
#include "memory/literal.hpp"

namespace lang::ast {
  /** @brief AST leaf wrapping the `()` unit literal, distinct from type::UnitNode (the type-system unit type). */
  class UnitNode : public Node {
  public:
    using Node::Node;

    /** @brief Return the node kind name, "unit". */
    std::string node_name() const override { return "unit"; }

    /**
     * @brief Print this node as `()` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Process this node. Always succeeds; there is nothing to validate.
     * @param ctx Compilation context.
     * @return True.
     */
    bool process(Context &ctx) override;

    /** @brief Return the shared unit value. */
    value::Value& value() const override;
  };
}
