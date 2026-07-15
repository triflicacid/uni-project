#pragma once

#include "ast/node.hpp"

namespace lang::ast {
  /** @brief Represents an array literal `[e1, e2, ...]`, holding its element expressions appended incrementally via `add`. */
  class ArrayLiteralNode : public Node {
    std::deque<std::unique_ptr<Node>> elements_;

  public:
    using Node::Node;

    /** @brief Return the node kind name, "array literal". */
    std::string node_name() const override { return "array literal"; }

    /**
     * @brief Append an element expression to this array.
     * @param node Element to append.
     */
    void add(std::unique_ptr<Node> node);

    /**
     * @brief Print this literal as `[e1, e2, ...]` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this literal and its elements in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Collect symbols from each element.
     * @param messages Message list to report errors into.
     * @param registry Registry to collect symbols into.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Process each element, unifying/checking their types against a type hint or each other, and deduce the array's element type.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code for each element.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(lang::Context &ctx) override;
  };
}
