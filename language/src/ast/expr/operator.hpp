#pragma once

#include "node.hpp"

namespace lang::ops {
  class Operator;
}

namespace lang::ast::expr {
  // define an operator, operator is token.image
  class OperatorNode : public Node {
  protected:
    std::optional<std::reference_wrapper<const ops::Operator>> op_; // resolves operator, set in ::process

    // 'creates' the functional type signature
    virtual const type::FunctionNode& signature() = 0;

    // overload to add additional processing, useful for child classes
    virtual bool _process(Context& ctx) const = 0;

  public:
    using Node::Node;

    const type::Node& type() const override final;

    bool process(lang::Context &ctx) override final;

    bool load(lang::Context &ctx) const override;
  };

  // represents lhs `op` rhs
  class BinaryOperatorNode : public OperatorNode {
    std::unique_ptr<Node> lhs_;
    std::unique_ptr<Node> rhs_;

    const type::FunctionNode& signature() override;

    bool _process(lang::Context &ctx) const override;

  public:
    BinaryOperatorNode(lexer::Token token, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs)
    : OperatorNode(std::move(token)), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    std::string name() const override { return "binary operator"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool load(lang::Context &ctx) const override;
  };

  // represents `op` expr
  class UnaryOperatorNode : public OperatorNode {
    std::unique_ptr<Node> expr_;
    std::optional<std::reference_wrapper<type::Node>> type_;

    const type::FunctionNode& signature() override;

    bool _process(lang::Context &ctx) const override;

  public:
    UnaryOperatorNode(lexer::Token token, std::unique_ptr<Node> expr)
    : OperatorNode(std::move(token)), expr_(std::move(expr)) {}

    std::string name() const override { return "unary operator"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool load(lang::Context &ctx) const override;
  };
}
