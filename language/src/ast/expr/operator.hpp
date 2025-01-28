#pragma once

#include "node.hpp"

namespace lang::ast::expr {
  struct OperatorInfo {
    uint8_t precedence; // operator precedence, 1 is loosest
    bool right_associative;
  };

  // define an operator, operator is token.image
  class OperatorNode : public Node {
  protected:
    std::optional<std::reference_wrapper<type::Node>> type_; // type of operator expression, populated after parsing during semantic check

  public:
    using Node::Node;

    const type::Node& type() const override final;
  };

  // represents lhs `op` rhs
  class BinaryOperatorNode : public OperatorNode {
    const std::unique_ptr<const Node> lhs_;
    const std::unique_ptr<const Node> rhs_;

  public:
    BinaryOperatorNode(lexer::Token token, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
    : OperatorNode(std::move(token)), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    std::string name() const override { return "binary operator"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;
  };

  // represents `op` expr
  class UnaryOperatorNode : public OperatorNode {
    const std::unique_ptr<const Node> expr_;
    std::optional<std::reference_wrapper<type::Node>> type_;

  public:
    UnaryOperatorNode(lexer::Token token, std::unique_ptr<Node> expr)
    : OperatorNode(std::move(token)), expr_(std::move(expr)) {}

    std::string name() const override { return "unary operator"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;
  };
}
