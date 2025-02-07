#pragma once

#include <deque>
#include "ast/node.hpp"

namespace lang::ast {
  class FunctionCallNode : public Node {
    std::unique_ptr<Node> subject_;
    std::deque<std::unique_ptr<Node>> args_;

  public:
    FunctionCallNode(lexer::Token token, std::unique_ptr<Node> subject, std::deque<std::unique_ptr<Node>> args)
    : Node(std::move(token)), subject_(std::move(subject)), args_(std::move(args)) {}

    std::string node_name() const override { return "function call"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

//    bool process(Context &ctx) override;

//    bool resolve_lvalue(Context& ctx) override;

//    bool resolve_rvalue(Context& ctx) override;
  };
}
