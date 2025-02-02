#pragma once

#include "node.hpp"
#include "operators/info.hpp"

namespace lang::ops {
  class Operator;
}

namespace lang::symbol {
  class Symbol;
}

namespace lang::ast::expr {
  // define an operator
  class OperatorNode : public Node {
  protected:
    lexer::Token op_symbol_; // token of our actual symbol
    std::deque<std::unique_ptr<Node>> args_;

    Node& lhs_() const;
    Node& rhs_() const;

    // loads all args_ and checks that rvalue, return success
    bool load_and_check_rvalue(Context& ctx) const;

  public:
    OperatorNode(lexer::Token token, lexer::Token symbol, std::deque<std::unique_ptr<Node>> args)
      : Node(std::move(token)), op_symbol_(std::move(symbol)), args_(std::move(args)) {
      if (!args_.empty()) token_end(args_.back()->token_end());
    }

    virtual const std::string& symbol() const { return op_symbol_.image; }

    std::string node_name() const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override final;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override final;

    bool process(lang::Context &ctx) override;

    // create a unary overloadable operator node
    static std::unique_ptr<OperatorNode> unary(lexer::Token token, std::unique_ptr<Node> expr);

    // create a binary overloadable operator node
    static std::unique_ptr<OperatorNode> binary(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);
  };

  // define an overlodable operator - search operatorX(...)
  class OverloadableOperatorNode : public OperatorNode {
    std::optional<std::reference_wrapper<const type::FunctionNode>> signature_; // signature, set in ::process
    std::optional<std::reference_wrapper<const ops::Operator>> op_; // resolves operator, set in ::process

  public:
    using OperatorNode::OperatorNode;

    // "operatorX(...)"
    std::string to_string() const;

    bool process(lang::Context &ctx) override;

    std::unique_ptr<value::Value> get_value(lang::Context &ctx) const override;

    std::unique_ptr<value::Value> load(lang::Context &ctx) const override;
  };

  // represents (type) expr
  class CastOperatorNode : public OperatorNode {
    const type::Node& target_;
    std::string symbol_; // (type) symbol, stored for future reference (printing)

  public:
    CastOperatorNode(lexer::Token token, const type::Node& target, std::unique_ptr<Node> expr);

    const std::string& symbol() const override { return symbol_; }

    std::unique_ptr<value::Value> get_value(lang::Context &ctx) const override;

    std::unique_ptr<value::Value> load(lang::Context &ctx) const override;
  };

  // represents lhs.rhs
  class DotOperatorNode : public OperatorNode {
    std::string resolved_; // resolved name
    std::optional<std::reference_wrapper<symbol::Symbol>> symbol_; // resolved symbol, may still be empty after ::process

  public:
    DotOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    bool process(lang::Context &ctx) override;

    std::unique_ptr<value::Value> get_value(lang::Context &ctx) const override;

    std::unique_ptr<value::Value> load(lang::Context &ctx) const override;
  };
}
