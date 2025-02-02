#pragma once

#include <deque>
#include "node.hpp"
#include "symbol/registry.hpp"

namespace lang::ast {
  class BlockNode : public Node, public ContainerNode {
    std::deque<std::unique_ptr<Node>> lines_;
    bool scope_ = true; // add a new scope
    std::unique_ptr<symbol::Registry> registry_; // local registry, NULL if !scope_

  public:
    using Node::Node;

    std::string node_name() const override { return "block"; }

    void add(std::unique_ptr<Node> ast_node) override;

    void add(std::deque<std::unique_ptr<Node>> ast_nodes) override;

    // is this block the start of a new scope?
    void add_new_scope(bool b) { scope_ = b; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List& messages, symbol::Registry &registry) override;

    bool process(lang::Context &ctx) override;
  };
}