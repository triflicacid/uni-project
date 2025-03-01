#pragma once

#include "ast/node.hpp"
#include "operators/info.hpp"
#include "optional_ref.hpp"
#include "memory/storage_location.hpp"
#include <unordered_set>

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

    Node& arg(int i) const;

    // expect node to be an r/lvalue, return if error
    bool expect_arg_lrvalue(int i, message::List& messages, bool expect_lvalue) const;

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

    bool process(Context &ctx) override;

    // create a unary overloadable operator node
    static std::unique_ptr<OperatorNode> unary(lexer::Token token, std::unique_ptr<Node> expr);

    // create a binary overloadable operator node
    static std::unique_ptr<OperatorNode> binary(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);
  };

  // define an overlodable operator - search operatorX(...)
  class OverloadableOperatorNode : public OperatorNode {
    optional_ref<const type::FunctionNode> signature_; // signature, set in ::process
    optional_ref<const ops::Operator> op_; // resolves operator, set in ::process
    bool special_pointer_op_ = false; // track if +/- on a pointer as we need to do something special

  public:
    using OperatorNode::OperatorNode;

    // "operatorX(...)"
    std::string to_string() const;

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };

  // represents `expr as type`
  class CastOperatorNode : public OperatorNode {
    const type::Node& target_;
    bool sudo_ = false;

  public:
    CastOperatorNode(lexer::Token token, lexer::Token symbol, const type::Node& target, std::unique_ptr<Node> expr, bool sudo = false);

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };

  // represents lhs.rhs
  class DotOperatorNode : public OperatorNode {
    std::string property_;

  public:
    DotOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    bool process(Context &ctx) override;

    bool resolve(Context& ctx) override;

    bool generate_code(Context &ctx) override;
  };

  // represents lhs = rhs
  class AssignmentOperatorNode : public OperatorNode {
  public:
    AssignmentOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };

  // represents &expr
  // returns/computes the address of the given lvalue
  class AddressOfOperatorNode : public OperatorNode {
  public:
    AddressOfOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> expr);

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };

  // represents *expr
  // gets the value stored in the given lvalue
  class DereferenceOperatorNode : public OperatorNode {
  public:
    DereferenceOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> expr);

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };

  // represents expr(<args>)
  // i.e., a function call but generalised
  class FunctionCallOperatorNode : public OperatorNode {
    std::unique_ptr<Node> subject_; // note, args_ only contains things in (...)
    optional_ref<const type::FunctionNode> signature_;
    optional_ref<const symbol::Symbol> symbol_; // populated if calling a symbol

  public:
    FunctionCallOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> subject, std::deque<std::unique_ptr<Node>> args);

    std::string node_name() const override { return "function call"; }

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };

  // represents sizeof expr
  class SizeOfOperatorNode : public OperatorNode {
    optional_ref<const type::Node> type_; // expr_ may be nullptr

  public:
    SizeOfOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> expr);
    SizeOfOperatorNode(lexer::Token token, lexer::Token symbol, const type::Node& type);

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };

  // represents lhs[rhs]
  class SubscriptOperatorNode : public OperatorNode {
    optional_ref<const type::FunctionNode> signature_;
    optional_ref<const ops::Operator> op_; // overloaded operator, in case of non-pointer behaviour

  public:
    SubscriptOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };

  // represents a binary logical operator || and &&
  class LazyLogicalOperator : public OperatorNode {
    bool and_; // && or ||

  public:
    LazyLogicalOperator(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    bool process(Context &ctx) override;

    bool generate_code(Context &ctx) override;
  };
}
