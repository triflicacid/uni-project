#pragma once

#include "node.hpp"
#include "memory/literal.hpp"

namespace lang::ast::expr {
  // describe a float or int literal
  // essentially a node wrapper around memory::Literal
  class LiteralNode : public Node {
    const memory::Literal& lit_;

  public:
    LiteralNode(lexer::Token token, const memory::Literal& lit) : Node(std::move(token)), lit_(lit) {}
    LiteralNode(lexer::Token token, const ast::type::Node& type)
      : Node(std::move(token)), lit_(memory::Literal::get(type, token_.value)) {}

    std::string name() const override { return "literal"; }

    const memory::Literal& get() const { return lit_; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(lang::Context &ctx) override;

    std::unique_ptr<value::Value> get_value(lang::Context &ctx) const override;

    std::unique_ptr<value::Value> load(lang::Context &ctx) const override;
  };
}
