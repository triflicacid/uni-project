#pragma once

#include "node.hpp"
#include "block.hpp"

namespace lang::ast {
  class NamespaceNode : public Node, public ContainerNode {
    std::deque<lexer::Token> names_; // <name1>.<name2> ...
    std::deque<std::unique_ptr<Node>> lines_;
    std::unique_ptr<symbol::Registry> registry_; // local registry
    std::optional<symbol::SymbolId> id_; // IF of *topmost* namespace

  public:
    NamespaceNode(lexer::Token token, std::deque<lexer::Token> names) : Node(std::move(token)), names_(std::move(names)) {}
    NamespaceNode(lexer::Token token, std::deque<lexer::Token> names, std::deque<std::unique_ptr<Node>> lines)
        : Node(std::move(token)), names_(std::move(names)), lines_(std::move(lines)) {}

    std::string node_name() const override { return "namespace"; }

    std::string name() const;

    void add(std::unique_ptr<Node> ast_node) override;

    void add(std::deque<std::unique_ptr<Node>> ast_nodes) override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };
}
