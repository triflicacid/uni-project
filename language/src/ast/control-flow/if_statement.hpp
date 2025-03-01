#pragma once

#include "language/src/ast/node.hpp"
#include "optional_ref.hpp"

namespace lang::ast {
  class IfStatementNode : public Node {
    unsigned int id_; // id for creating if-else blocks
    std::unique_ptr<Node> guard_;
    std::unique_ptr<Node> then_;
    std::optional<std::unique_ptr<Node>> else_;
    std::optional<lexer::Token> else_token_; // `else` token

  public:
    IfStatementNode(lexer::Token token, std::unique_ptr<Node> guard, std::unique_ptr<Node> then_body, std::optional<lexer::Token> else_token, std::optional<std::unique_ptr<Node>> else_body);

    std::string node_name() const override { return "if statement"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool always_returns() const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };
}
