#pragma once

#include "ast/node.hpp"
#include "operators/info.hpp"
#include "optional_ref.hpp"

namespace lang::ops {
  class Operator;
}

namespace lang::symbol {
  class Symbol;
}

namespace lang::ast::type {
  class FunctionNode;
}

namespace lang::ast {
  // define an operator
  class OperatorNode : public Node {
  protected:
    lexer::Token op_symbol_; // token of our actual symbol
    std::deque<std::unique_ptr<Node>> args_;

    Node& lhs_() const;
    Node& rhs_() const;
    
    optional_ref<const value::Value> get_arg_rvalue(Context& ctx, int i) const;
    optional_ref<const value::Value> get_arg_lvalue(Context& ctx, int i) const;

  public:
    OperatorNode(lexer::Token token, lexer::Token symbol, std::deque<std::unique_ptr<Node>> args)
      : Node(std::move(token)), op_symbol_(std::move(symbol)), args_(std::move(args)) {
      if (!args_.empty()) token_end(args_.back()->token_end());
    }

    virtual const std::string& symbol() const { return op_symbol_.image; }

    std::string node_name() const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

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

    bool resolve_rvalue(Context& ctx) override;
  };

  // represents `expr as type`
  class CastOperatorNode : public OperatorNode {
    const type::Node& target_;

  public:
    CastOperatorNode(lexer::Token token, const type::Node& target, std::unique_ptr<Node> expr);

    bool process(lang::Context &ctx) override;

    bool resolve_rvalue(lang::Context &ctx) override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;
  };

  // represents (type) expr
  // this is used for basic C-style casting (only primitive types are permitted)
  class CStyleCastOperatorNode : public OperatorNode {
    const type::Node& target_;
    std::string symbol_; // (type) symbol, stored for future reference (printing)

  public:
    CStyleCastOperatorNode(lexer::Token token, const type::Node& target, std::unique_ptr<Node> expr);

    const std::string& symbol() const override { return symbol_; }

    bool process(lang::Context &ctx) override;

    bool resolve_rvalue(lang::Context &ctx) override;
  };

  // represents lhs.rhs
  class DotOperatorNode : public OperatorNode {
    std::string attr_; // resolved attribute name

  public:
    DotOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    bool process(lang::Context &ctx) override;

    bool resolve_lvalue(lang::Context &ctx) override;

    bool resolve_rvalue(lang::Context &ctx) override;
  };

  // represents lhs = rhs
  class AssignmentOperatorNode : public OperatorNode {
  public:
    AssignmentOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    bool process(lang::Context &ctx) override;

    bool resolve_lvalue(lang::Context &ctx) override;

    bool resolve_rvalue(lang::Context &ctx) override;
  };

  // represents registerof expr
  // returns the register offset of the given symbol
  class RegisterOfOperatorNode : public OperatorNode {
  public:
    RegisterOfOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> expr);

    bool process(lang::Context &ctx) override;

    bool resolve_rvalue(lang::Context &ctx) override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;
  };

  // represents &expr
  // returns/computes the address of the given lvalue
  class AddressOfOperatorNode : public OperatorNode {
  public:
    AddressOfOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> expr);

    bool process(lang::Context &ctx) override;

    bool resolve_rvalue(lang::Context &ctx) override;
  };

  // represents *expr
  // gets the value stored in the given lvalue
  class DereferenceOperatorNode : public OperatorNode {
  public:
    DereferenceOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> expr);

    bool process(lang::Context &ctx) override;

    bool resolve_rvalue(lang::Context &ctx) override;
  };
}
