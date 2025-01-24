#pragma once

#include "node.hpp"
#include "ast/types/node.hpp"

namespace lang::ast {
  class SymbolDeclarationNode : public Node {
    const type::Node& type_;

  public:
    SymbolDeclarationNode(lexer::Token token, const type::Node& type) : Node(std::move(token)), type_(type) {}

    std::string name() const override { return "symbol declaration"; }

    const type::Node& type() const { return type_; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(lang::Context &ctx) override;
  };
}
