#pragma once

#include <string>
#include "ast/types/node.hpp"

namespace lang::symbol {
  using SymbolId = uint32_t;

  // represent the category of a symbol
  enum class Category {
    Ordinary, // ordinary symbol, stack-based, offset determined by StackManager
    Argument, // argument, point to index
    Function, // globally-places function (block-bound)
  };

  std::string category_to_string(Category category);

  // represents a value bound to a name
  // includes namespaces, variables, and functions (which are variables with function types)
  class Symbol {
    lexer::Token token_; // origin token (top-level name)
    std::optional<std::reference_wrapper<const Symbol>> parent_; // parent symbol (i.e., namespace)
    SymbolId id_ = -1;
    Category category_;
    int ref_count_ = 0; // count how many times we were referenced by a SymbolReferenceNode

  public:
    explicit Symbol(lexer::Token name);
    Symbol(lexer::Token name, Category category);

    virtual ~Symbol() = default;

    const lexer::Token& token() const { return token_; }

    const Category& category() const { return category_; }

    const std::string& name() const { return token_.image; }

    // generate fully-qualified name (by tracking parents)
    std::string full_name() const;

    const std::optional<std::reference_wrapper<const Symbol>>& parent() const { return parent_; }

    void set_parent(const Symbol& parent) { parent_ = parent; }

    uint32_t id() const { return id_; }

    int ref_count() const { return ref_count_; }

    // increment our ref_count
    void inc_ref() { ref_count_++; }

    virtual const ast::type::Node& type() const = 0;
  };
}
