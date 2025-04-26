#pragma once

#include "ast/node.hpp"
#include "memory/literal.hpp"

namespace lang::ast {
  // describe a scalar literal
  // essentially a node wrapper around memory::Literal
  class LiteralNode : public Node {
    optional_ref<const memory::Literal> lit_;
    optional_ref<const type::Node> suffix_; // type suffix? this overrides type hinting

    // get numeric type to cast to
    const type::Node& get_target_numeric_type() const;

  public:
    LiteralNode(lexer::Token token) : Node(std::move(token)) {}
    LiteralNode(lexer::Token token, const memory::Literal& lit) : Node(std::move(token)), lit_(lit) {}

    // set type suffix
    void suffix(const type::Node& type) { suffix_ = type; }

    std::string node_name() const override { return "literal"; }

    const memory::Literal& get() const;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(Context &ctx) override;
  };
}
