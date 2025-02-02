#pragma once

#include <deque>
#include "node.hpp"

namespace lang::ast {
  class ProgramNode : public Node, public ContainerNode {
    std::deque<std::unique_ptr<Node>> lines_;

  public:
    using Node::Node;

    std::string node_name() const override { return "program"; }

    void add(std::unique_ptr<Node> ast_node) override;

    void add(std::deque<std::unique_ptr<Node>> ast_nodes) override;

    const Node& back() const { return *lines_.back(); }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(lang::Context &ctx) override;
  };
}