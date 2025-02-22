#pragma once

#include "language/src/ast/node.hpp"
#include "optional_ref.hpp"

namespace lang::ast {
  class WhileStatementNode : public Node {
    unsigned int id_; // id for creating while blocks
    std::unique_ptr<Node> guard_;
    std::unique_ptr<Node> body_;

  public:
    WhileStatementNode(lexer::Token token, std::unique_ptr<Node> guard, std::unique_ptr<Node> body);

    std::string node_name() const override { return "while statement"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool always_returns() const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(lang::Context &ctx) override;

    bool generate_code(lang::Context &ctx) const override;

    // return label for the guard/conditional block
    std::string guard_label() const;

    // return label for the while loop's body
    std::string body_label() const;

    // return label for the end of the while loop
    std::string end_label() const;
  };
}
