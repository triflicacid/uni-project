#pragma once

#include "node.hpp"
#include "ast/types/node.hpp"
#include "symbol/symbol.hpp"

namespace lang::ast {
  class SymbolDeclarationNode : public Node {
    const type::Node& type_;
    bool arg_;
    symbol::SymbolId id_; // ID of created symbol

  public:
    SymbolDeclarationNode(lexer::Token token, const type::Node& type, bool is_arg = false) : Node(std::move(token)), type_(type), arg_(is_arg) {}

    std::string name() const override { return arg_ ? "argument declaration" : "symbol declaration"; }

    void set_arg(bool b) { arg_ = b; }

    const type::Node& type() const { return type_; }

    // get ID of created function
    symbol::SymbolId id() const { return id_; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(lang::Context &ctx) override;
  };
}
