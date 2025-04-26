#pragma once
#include "../node.hpp"
#include "symbol/registry.hpp"
#include "../symbol_declaration.hpp"
#include <unordered_set>

namespace lang::ast {
  class FunctionBaseNode : public Node {
    lexer::Token name_;
    std::deque<std::unique_ptr<SymbolDeclarationNode>> params_; // note, assume that arg types and param types are equivalent
    symbol::SymbolId id_; // ID of created/referencing function (created in ::collate_registry)
    bool defined_ = false; // has this function been created/defined

    // validate the parameters
    bool validate_params(message::List& messages);

  protected:
    const type::FunctionNode& type_;
    bool generate_code_ = true; // `false` tells us to skip code generation
    bool define_function_ = true; // define ourself as a `function` symbol? If false, don't define ourself.

    // process child, new function as it is between function frame push/pop
    virtual bool _process(Context& ctx) = 0;

    // code generation once stack frame etc. have been set up but prior to cleanup
    // called by this::define
    virtual bool _generate_code(Context& ctx) = 0;

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

    const type::FunctionNode& type() const { return type_; }

    bool writes_to_ret() const override;

    // return set of 0-indexed arguments to ignore, pass to ops::call_function
    std::unordered_set<int> get_args_to_ignore() const;

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    bool process(Context &ctx) override final;

    bool generate_code(lang::Context &ctx) override final;

    // define (generate code for) our function
    // return if success
    bool define(Context& ctx);
  };
}