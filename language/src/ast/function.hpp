#pragma once
#include "node.hpp"
#include "block.hpp"
#include "symbol_declaration.hpp"
#include "types/function.hpp"

namespace lang::ast {
  class FunctionNode : public Node {
    std::unique_ptr<type::FunctionNode> type_;
    std::deque<std::unique_ptr<SymbolDeclarationNode>> params_; // note, assume that arg types and param types are equivalent
    std::optional<std::unique_ptr<BlockNode>> body_; // body is optional, empty = 'not implemented'

  public:
    FunctionNode(lexer::Token token, std::unique_ptr<type::FunctionNode> type, std::deque<std::unique_ptr<SymbolDeclarationNode>> params, std::optional<std::unique_ptr<BlockNode>> body)
        : Node(std::move(token)), type_(std::move(type)), params_(std::move(params)), body_(std::move(body)) {}

    std::string name() const override { return "function"; }

    // return if we are implemented, i.e., have a body
    bool is_implemented() const { return body_.has_value(); }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool process(lang::Context &ctx) override;
  };
}