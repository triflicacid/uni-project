#pragma once

#include "ast/node.hpp"

namespace lang::ast {
  class ArrayLiteralNode : public Node {
    std::deque<std::unique_ptr<Node>> elements_;

  public:
    using Node::Node;

    std::string node_name() const override { return "array literal"; }

    // add new element to this array
    void add(std::unique_ptr<Node> node);

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(Context &ctx) override;

    bool generate_code(lang::Context &ctx) override;
  };
}
