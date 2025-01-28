#pragma once

#include "node.hpp"

namespace lang::ast::expr {
  // describe a float or int literal
  class LiteralNode : public Node {
    const type::Node& type_; // either Float or Int

  public:
    LiteralNode(lexer::Token token, const type::Node& type) : Node(std::move(token)), type_(type) {}

    const type::Node& type() const override { return type_; }

    std::string name() const override { return "literal"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    // return string format of the numeric literal we represent
    std::string to_string() const;

    // return data buffer
    uint64_t get() const { return token_.value; }

    bool process(lang::Context &ctx) override;

    bool load(lang::Context &ctx) const override;
  };
}
