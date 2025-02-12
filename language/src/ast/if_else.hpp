#pragma once

#include "node.hpp"
#include "optional_ref.hpp"

namespace lang::ast {
  class IfElseNode : public Node {
    std::unique_ptr<Node> guard_;
    std::unique_ptr<Node> then_;
    std::optional<std::unique_ptr<Node>> else_;
    optional_ref<value::Value> result_; // cache resulting value of statement

  public:
    IfElseNode(lexer::Token token, std::unique_ptr<Node> guard, std::unique_ptr<Node> then_body, std::optional<std::unique_ptr<Node>> else_body);

    std::string node_name() const override { return "if statement"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool always_returns() const override;

    const value::Value& value() const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;
  };
}
