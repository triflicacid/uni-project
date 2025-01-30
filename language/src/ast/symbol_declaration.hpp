#pragma once

#include "node.hpp"
#include "ast/types/node.hpp"
#include "symbol/symbol.hpp"
#include "expr.hpp"

namespace lang::ast {
  class SymbolDeclarationNode : public Node {
    std::optional<std::reference_wrapper<const type::Node>> type_; // optional if type deduction required
    bool arg_;
    symbol::SymbolId id_; // ID of created symbol
    std::optional<std::unique_ptr<ast::ExprNode>> assignment_; // optional assignment

  public:
    SymbolDeclarationNode(lexer::Token token, std::optional<std::reference_wrapper<const type::Node>> type, bool is_arg = false)
      : Node(std::move(token)), type_(std::move(type)), arg_(is_arg) {}

    SymbolDeclarationNode(lexer::Token token, std::optional<std::reference_wrapper<const type::Node>> type, std::optional<std::unique_ptr<ast::ExprNode>> assignment, bool is_arg = false)
      : Node(std::move(token)), type_(std::move(type)), arg_(is_arg), assignment_(std::move(assignment)) {}

    std::string name() const override { return arg_ ? "argument declaration" : "symbol declaration"; }

    void set_arg(bool b) { arg_ = b; }

    bool is_assignment() const { return assignment_.has_value(); }

    // make this into an assignment also
    void set_assignment(std::unique_ptr<ast::ExprNode> body) { assignment_ = std::move(body); }

    const type::Node& type() const;

    // get ID of created function
    symbol::SymbolId id() const { return id_; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(lang::Context &ctx) override;
  };
}
