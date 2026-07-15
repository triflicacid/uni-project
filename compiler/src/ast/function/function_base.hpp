#pragma once
#include "../node.hpp"
#include "symbol/registry.hpp"
#include "../symbol_declaration.hpp"
#include <unordered_set>

namespace lang::ast {
  /**
   * @brief Abstract base for anything function-shaped: plain function definitions and operator definitions.
   *
   * Holds the function's name token, its parameter declarations (assumed to
   * match the signature's parameter types 1:1), the symbol id created for it,
   * and a reference to its type::FunctionNode signature. `process`/`generate_code`
   * are template methods (final) that set up the function's scope/stack frame
   * and delegate the function-specific work to the `_process`/`_generate_code`
   * hooks implemented by subclasses.
   */
  class FunctionBaseNode : public Node {
    lexer::Token name_;
    std::deque<std::unique_ptr<SymbolDeclarationNode>> params_; // note, assume that arg types and param types are equivalent
    symbol::SymbolId id_; // ID of created/referencing function (created in ::collate_registry)
    bool defined_ = false; // has this function been created/defined

    /**
     * @brief Check that no two parameters share the same name (ignoring the discard symbol `_`).
     * @param messages Message list to report a duplicate-parameter error into.
     * @return True if all parameter names are distinct.
     */
    bool validate_params(message::List& messages);

  protected:
    const type::FunctionNode& type_;
    bool generate_code_ = true; // `false` tells us to skip code generation
    bool define_function_ = true; // define ourself as a `function` symbol? If false, don't define ourself.

    /**
     * @brief Process the function's specific contents, called between the function's frame push/pop.
     * @param ctx Compilation context.
     * @return True on success.
     */
    virtual bool _process(Context& ctx) = 0;

    /**
     * @brief Generate code for the function's specific contents, once its stack frame is set up but before cleanup.
     * @param ctx Compilation context.
     * @return True on success.
     */
    virtual bool _generate_code(Context& ctx) = 0;

    /** @brief Return the text preceding the parameter list in `print_code`, e.g. `"func <name>"`. */
    virtual std::string block_prefix() const = 0;

  public:
    /**
     * @brief Construct a function-shaped node.
     * @param token Start token of the declaration.
     * @param name Token holding the function's name.
     * @param type Function signature.
     * @param params Parameter declarations.
     */
    FunctionBaseNode(lexer::Token token, lexer::Token name, const type::FunctionNode& type, std::deque<std::unique_ptr<SymbolDeclarationNode>> params);

    /** @brief Return the number of parameters. */
    size_t params() const { return params_.size(); }

    /** @brief Test whether this function has a body (false means an extern-style declaration). */
    virtual bool is_implemented() const = 0;

    /** @brief Return the token holding the function's name. */
    const lexer::Token& name() const { return name_; }

    /**
     * @brief Return the ith parameter declaration.
     * @param i Index of the parameter.
     * @return The parameter's declaration node.
     */
    const SymbolDeclarationNode& param(unsigned int i) const { return *params_[i]; }

    /** @brief Return the id of the symbol created for this function. */
    symbol::SymbolId id() const { return id_; }

    /** @brief Return this function's signature. */
    const type::FunctionNode& type() const { return type_; }

    /** @brief Always true: calling into this function may write its result to `$ret`. */
    bool writes_to_ret() const override;

    /** @brief Return the 0-indexed positions of parameters named `_` (discarded), for `ops::call_function` to skip. */
    std::unordered_set<int> get_args_to_ignore() const;

    /**
     * @brief Print this function's signature (prefix, parameter list, and return type) as source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this function's name and parameters in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Validate parameter names and register this function as a symbol (unless it's an unimplemented declaration already satisfied elsewhere).
     * @param messages Message list to report errors into.
     * @param registry Registry to create the function's symbol in.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Enter the function's scope, bind parameter symbols to their stack locations, run the subclass's `_process`, then verify the function returns if required.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override final;

    /**
     * @brief Trigger definition (code generation) of this function if the toolchain is configured to always define symbols eagerly.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(lang::Context &ctx) override final;

    /**
     * @brief Generate code for this function's body (idempotent; a no-op if already defined), including its prologue/epilogue.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool define(Context& ctx);
  };
}