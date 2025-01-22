#pragma once

#include <deque>
#include "node.hpp"

namespace lang::ast {
  class BlockNode : public Node {
    std::deque<std::unique_ptr<Node>> lines_;
    bool scope_ = true; // add a new scope

  public:
    using Node::Node;

    std::string name() const override { return "block"; }

    void add(std::unique_ptr<Node> ast_node);

    // is this block the start of a new scope?
    void add_new_scope(bool b) { scope_ = b; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(lang::Context &ctx) override;
  };
}