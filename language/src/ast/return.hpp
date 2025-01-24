#pragma once

#include "node.hpp"
#include "ast/expr/node.hpp"

namespace lang::ast {
  class ReturnNode : public Node {
    std::optional<std::unique_ptr<expr::Node>> expr_;

  public:
    explicit ReturnNode(lexer::Token token) : Node(std::move(token)) {}
    ReturnNode(lexer::Token token, std::optional<std::unique_ptr<expr::Node>> expr) : Node(std::move(token)), expr_(std::move(expr)) {}

    std::string name() const override { return "return"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;
  };
}
