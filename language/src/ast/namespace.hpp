#pragma once

#include "node.hpp"
#include "block.hpp"

namespace lang::ast {
  class NamespaceNode : public Node {
    std::unique_ptr<BlockNode> body_;

  public:
    NamespaceNode(lexer::Token token, std::unique_ptr<BlockNode> body)
        : Node(std::move(token)), body_(std::move(body)) {}

    std::string name() const override { return "namespace"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(lang::Context &ctx) override;
  };
}
