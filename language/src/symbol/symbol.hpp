 #pragma once

#include <string>
#include "ast/types/node.hpp"

namespace lang::symbol {
  using SymbolId = uint32_t;

  // represent the category of a symbol
  enum class Category {
    Ordinary, // ordinary symbol, stack-based, offset determined by StackManager
    Argument, // argument, point to index
    Function, // globally-placed function (block-bound)
    Namespace,
  };

  std::string category_to_string(Category category);

  // represents a value bound to a name
  // includes namespaces, variables, and functions (which are variables with function types)
  class Symbol {
    lexer::Token token_; // origin token (top-level name)
    std::optional<std::reference_wrapper<const Symbol>> parent_; // parent symbol (i.e., namespace)
    SymbolId id_ = -1;
    Category category_;
    const ast::type::Node& type_;
    bool constant_ = false;
    bool assigned_ = false; // record if we have been assigned to

  public:
    Symbol(const Symbol&) = delete;
    Symbol(lexer::Token name, const ast::type::Node& type_);
    Symbol(lexer::Token name, Category category, const ast::type::Node& type_);

    virtual ~Symbol() = default;

    const lexer::Token& token() const { return token_; }

    const Category& category() const { return category_; }

    const std::string& name() const { return token_.image; }

    void make_constant() { constant_ = true; }

    bool is_constant() const { return constant_; }

    // generate fully-qualified name (by tracking parents)
    std::string full_name() const;

    const std::optional<std::reference_wrapper<const Symbol>>& parent() const { return parent_; }

    void set_parent(const Symbol& parent) { parent_ = parent; }

    uint32_t id() const { return id_; }

    const ast::type::Node& type() const { return type_; }

    // ensure that this symbol is defined
    // return success
    virtual bool define(Context& ctx) const;
  };

  // create a new namespace
  std::unique_ptr<Symbol> create_namespace(const lexer::Token& name);
}
