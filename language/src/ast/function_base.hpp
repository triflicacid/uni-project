#pragma once
#include "node.hpp"
#include "types/function.hpp"
#include "symbol/registry.hpp"

namespace lang::ast {
  class SymbolDeclarationNode;

  class FunctionBaseNode : public Node {
    std::unique_ptr<type::FunctionNode> type_;
    std::deque<std::unique_ptr<SymbolDeclarationNode>> params_; // note, assume that arg types and param types are equivalent

    // validate the parameters
    bool validate_params(message::List& messages);

  protected:
    std::unique_ptr<symbol::Registry> registry_;

    // processing once stack frame etc. have been set up
    virtual bool _process(lang::Context& ctx) = 0;

    // contents which precede argument list in print_code
    // e.g., `func <name>`
    virtual std::string block_prefix() const = 0;

  public:
    FunctionBaseNode(lexer::Token token, std::unique_ptr<type::FunctionNode> type, std::deque<std::unique_ptr<SymbolDeclarationNode>> params);

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(lang::Context &ctx) override final;
  };
}