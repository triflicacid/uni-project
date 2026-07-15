#pragma once

#include <deque>
#include "node.hpp"

namespace lang::ast {
  /** @brief Root AST node for an entire compiled unit, holding the ordered sequence of top-level statements. */
  class ProgramNode : public Node, public ContainerNode {
    std::deque<std::unique_ptr<Node>> lines_;

  public:
    using Node::Node;

    /** @brief Return the node kind name, "program". */
    std::string node_name() const override { return "program"; }

    /**
     * @brief Append a single top-level statement.
     * @param ast_node Statement to append.
     */
    void add(std::unique_ptr<Node> ast_node) override;

    /**
     * @brief Append a sequence of top-level statements.
     * @param ast_nodes Statements to append.
     */
    void add(std::deque<std::unique_ptr<Node>> ast_nodes) override;

    /** @brief Return the last top-level statement. */
    const Node& back() const { return *lines_.back(); }

    /**
     * @brief Print each top-level statement as source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this program and its top-level statements in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Collate all top-level symbols into one registry (allowing forward references), then process and resolve each statement.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code for each top-level statement, then emit a trailing program exit.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };
}