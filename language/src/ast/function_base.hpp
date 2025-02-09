#pragma once
#include "node.hpp"
#include "types/function.hpp"
#include "symbol/registry.hpp"

namespace lang::ast {
  class SymbolDeclarationNode;

  class FunctionBaseNode : public Node {
    lexer::Token name_;
    std::deque<std::unique_ptr<SymbolDeclarationNode>> params_; // note, assume that arg types and param types are equivalent
    symbol::SymbolId id_; // ID of created/referencing function (created in ::collate_registry)
    bool generate_code_ = true; // `false` tells us to skip code generation

    // validate the parameters
    bool validate_params(message::List& messages);

  protected:
    const type::FunctionNode& type_;
    std::unique_ptr<symbol::Registry> registry_;

    // processing once stack frame etc. have been set up
    virtual bool _process(lang::Context& ctx) = 0;

    // contents which precede argument list in print_code
    // e.g., `func <name>`
    virtual std::string block_prefix() const = 0;

  public:
    FunctionBaseNode(lexer::Token token, lexer::Token name, const type::FunctionNode& type, std::deque<std::unique_ptr<SymbolDeclarationNode>> params);

    size_t params() const { return params_.size(); }

    // checks if the function is implemented or not
    virtual bool is_implemented() const = 0;

    const lexer::Token& name() const { return name_; }

    const SymbolDeclarationNode& param(unsigned int i) const { return *params_[i]; }

    // get ID of created function
    symbol::SymbolId id() const { return id_; }

    const ast::type::FunctionNode& type() const { return type_; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(lang::Context &ctx) override final;
  };
}