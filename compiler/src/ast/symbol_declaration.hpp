#pragma once

#include "node.hpp"
#include "types/node.hpp"
#include "symbol/symbol.hpp"

namespace lang::ast {
  /**
   * @brief Represents a `name: type [= expr]` declaration, used for `let`/`const` statements and function parameters.
   *
   * The type may be omitted if it can be deduced from the assignment expression.
   * The `Category` distinguishes an ordinary variable, a function argument, and
   * a constant (which must be initialised and cannot be reassigned).
   */
  class SymbolDeclarationNode : public Node {
  public:
    /** @brief Kind of symbol this declaration introduces. */
    enum Category {
      Variable,
      Argument,
      Constant
    };

  private:
    lexer::Token name_; ///< Token holding the declared name.
    std::optional<std::reference_wrapper<const type::Node>> type_; ///< Explicit type, or empty if it must be deduced.
    Category category_ = Variable; ///< Kind of symbol this declaration introduces.
    symbol::SymbolId id_; ///< Id of the symbol created for this declaration.
    std::optional<std::unique_ptr<Node>> assignment_; ///< Initial-value expression, if present.

  public:
    /**
     * @brief Construct a declaration with an explicit or absent type and no assignment.
     * @param token Start token of the declaration.
     * @param name Token holding the declared name.
     * @param type Explicit type, or empty if it must be deduced.
     */
    SymbolDeclarationNode(lexer::Token token, lexer::Token name, std::optional<std::reference_wrapper<const type::Node>> type)
      : Node(std::move(token)), name_(std::move(name)), type_(std::move(type)) {}

    /**
     * @brief Construct a declaration with an explicit or absent type and an optional assignment.
     * @param token Start token of the declaration.
     * @param name Token holding the declared name.
     * @param type Explicit type, or empty if it must be deduced.
     * @param assignment Initial-value expression, if present.
     */
    SymbolDeclarationNode(lexer::Token token, lexer::Token name, std::optional<std::reference_wrapper<const type::Node>> type, std::optional<std::unique_ptr<Node>> assignment)
      : Node(std::move(token)), name_(std::move(name)), type_(std::move(type)), assignment_(std::move(assignment)) {}

    /** @brief Return the node kind name, depending on the declaration's category. */
    std::string node_name() const override;

    /** @brief Return the token holding the declared name. */
    const lexer::Token& name() const { return name_; }

    /**
     * @brief Set this declaration's initial-value expression.
     * @param expr Assignment expression.
     */
    void assign_to(std::unique_ptr<Node> expr) { assignment_ = std::move(expr); }

    /** @brief Return this declaration's category (variable, argument, or constant). */
    Category category() const { return category_; }

    /**
     * @brief Set this declaration's category.
     * @param c New category.
     */
    void set_category(Category c) { category_ = c; }

    /** @brief Return the declared symbol's type. Assumes it has been resolved (explicitly or by deduction). */
    const type::Node& type() const;

    /** @brief Return the id of the symbol created for this declaration. */
    symbol::SymbolId id() const { return id_; }

    /** @brief Test whether the assignment expression, if present, may write to `$ret`. */
    bool writes_to_ret() const override;

    /**
     * @brief Print this declaration as `let name: type [= expr]` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this declaration and its optional assignment in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Collect symbols from the assignment expression, if present.
     * @param messages Message list to report errors into.
     * @param registry Registry to collect symbols into.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Process the assignment expression (if any), deduce/validate the declared type, and create the symbol.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Allocate storage for the declared symbol and generate code for the assignment expression, if present.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(lang::Context &ctx) override;
  };
}
