#pragma once
#include "function_base.hpp"
#include "types/function.hpp"

namespace lang::ast {
  class BlockNode;

  class FunctionNode : public FunctionBaseNode {
    std::optional<std::unique_ptr<BlockNode>> body_; // body is optional, empty = 'not implemented'
    std::unique_ptr<symbol::Registry> registry_;

    bool _process(lang::Context &ctx) override;

    std::string block_prefix() const override { return "func " + name().image; }

  public:
    FunctionNode(lexer::Token token, lexer::Token, const type::FunctionNode& type, std::deque<std::unique_ptr<SymbolDeclarationNode>> params, std::optional<std::unique_ptr<BlockNode>> body);

    std::string node_name() const override { return "function"; }

    // return if we are implemented, i.e., have a body
    bool is_implemented() const override { return body_.has_value(); }

    bool always_returns() const override;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;
  };
}