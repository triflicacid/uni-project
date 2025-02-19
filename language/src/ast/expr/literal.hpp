#pragma once

#include "ast/node.hpp"
#include "memory/literal.hpp"

namespace lang::ast {
  // describe a float or int literal
  // essentially a node wrapper around memory::Literal
  class LiteralNode : public Node {
    optional_ref<const memory::Literal> lit_;

    // get numeric type to cast to
    const ast::type::Node& get_target_numeric_type() const;

  public:
    LiteralNode(lexer::Token token) : Node(std::move(token)) {}
    LiteralNode(lexer::Token token, const memory::Literal& lit) : Node(std::move(token)), lit_(lit) {}

    std::string node_name() const override { return "literal"; }

    const memory::Literal& get() const;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(lang::Context &ctx) override;

    bool generate_code(lang::Context &ctx) const override;
  };
}
