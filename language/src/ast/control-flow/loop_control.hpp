#pragma once

#include "language/src/ast/node.hpp"
#include "control-flow/loop_context.hpp"

namespace lang::ast {
  // defines a node which jumps to the start/end of the current loop
  class LoopControlNode : public Node {
  public:
    enum class Variant {
      Break,
      Continue,
    };

    Variant variant_; // jump to start (continue) or end (break)?
    optional_ref<const control_flow::LoopContext> loop_; // populated in ::process

  public:
    explicit LoopControlNode(lexer::Token token);
    LoopControlNode(lexer::Token token, Variant variant) : Node(std::move(token)), variant_(variant) {}

    std::string node_name() const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(lang::Context &ctx) override;

    bool generate_code(lang::Context &ctx) const override;
  };
}
