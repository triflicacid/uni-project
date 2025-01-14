#pragma once

#include "node.hpp"

namespace lang::ast::expr {
  enum class OperatorType {
    ASSIGNMENT,
    ADDITION,
    SUBTRACTION,
    MINUS,
    MULTIPLICATION,
    DIVISION,
  };

  struct OperatorInfo {
    uint8_t precedence; // operator precedence, 1 is loosest
    bool right_associative;
    OperatorType type;
  };

  class OperatorNode : public Node {
    OperatorType op_;

  protected:
    std::optional<std::reference_wrapper<type::Node>> type_; // type of operator expression, populated after parsing during semantic check

  public:
    OperatorNode(lexer::Token token, OperatorType op) : Node(std::move(token)), op_(op) {}

    OperatorType op() const { return op_; }

    const type::Node& type() const override final;
  };

  class BinaryOperatorNode : public OperatorNode {
    const std::unique_ptr<const Node> lhs_;
    const std::unique_ptr<const Node> rhs_;

  public:
    BinaryOperatorNode(lexer::Token token, OperatorType op, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
    : OperatorNode(std::move(token), op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    std::string name() const override { return "binary operator"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;
  };

  class UnaryOperatorNode : public OperatorNode {
    const std::unique_ptr<const Node> expr_;
    std::optional<std::reference_wrapper<type::Node>> type_;

  public:
    UnaryOperatorNode(lexer::Token token, OperatorType op, std::unique_ptr<Node> expr)
    : OperatorNode(std::move(token), op), expr_(std::move(expr)) {}

    std::string name() const override { return "unary operator"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;
  };
}
