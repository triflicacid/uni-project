#pragma once

#include "node.hpp"
#include "ast/expr/node.hpp"

namespace lang::ast {
  // this is a wrapper for ast::expr::Node, to ensure the entire expression is loaded
  class ExprNode : public Node {
    std::unique_ptr<expr::Node> expr_;

  public:
    ExprNode(std::unique_ptr<expr::Node> expr) : Node(expr->token()), expr_(std::move(expr)) {}

    std::string name() const override { return "expression"; }

    const type::Node& type() const { return expr_->type(); }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(lang::Context &ctx) override;
  };
}
