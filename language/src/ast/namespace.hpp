#pragma once

#include "node.hpp"
#include "block.hpp"

namespace lang::ast {
  class NamespaceNode : public Node, public ContainerNode {
    lexer::Token name_;
    std::deque<std::unique_ptr<Node>> lines_;
    std::unique_ptr<symbol::Registry> registry_; // local registry
    symbol::SymbolId id_; // ID of symbol created on our behalf

  public:
    NamespaceNode(lexer::Token token, lexer::Token name) : Node(std::move(token)), name_(std::move(name)) {}
    NamespaceNode(lexer::Token token, lexer::Token name, std::deque<std::unique_ptr<Node>> lines)
        : Node(std::move(token)), name_(std::move(name)), lines_(std::move(lines)) {}

    std::string node_name() const override { return "namespace"; }

    const lexer::Token& name() const { return name_; }

    void add(std::unique_ptr<Node> ast_node) override;

    void add(std::deque<std::unique_ptr<Node>> ast_nodes) override;

    // get ID of created symbol
    symbol::SymbolId id() const { return id_; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(lang::Context &ctx) override;
  };
}
