#pragma once

#include <unordered_map>
#include "symbol.hpp"
#include "assembly/basic_block.hpp"
#include "optional_ref.hpp"

namespace lang {
  namespace ast {
    class FunctionBaseNode;
  }

  namespace type {
    class Node;
  }
}

namespace lang::symbol {
  /**
   * @brief A local, not-yet-globally-inserted table of symbols for a single lexical construct, keyed by id and indexed by name.
   *
   * Symbols accumulate here during collate_registry before being merged wholesale
   * into a symbol::SymbolTable.
   */
  class Registry {
    friend class SymbolTable; // allow the symbol table to access us -- it will contain us

    std::unordered_map<SymbolId, std::unique_ptr<Symbol>> symbols_; ///< Symbols, keyed by id.
    std::unordered_map<std::string, std::deque<SymbolId>> names_; ///< Symbol ids, keyed by name; multiple ids support overloading.

    /**
     * @brief Records a symbol's id under its fully-qualified name, supporting multiple overloads per name.
     * @param symbol Symbol to index.
     */
    // helper - insert name-id into map
    void insert_name(const Symbol& symbol);

  public:
    Registry() = default;
    Registry(const Registry&) = delete;

    /**
     * @brief Returns the number of symbols currently registered.
     * @return Symbol count.
     */
    size_t size() const { return symbols_.size(); }

    /**
     * @brief Reports whether the registry holds no symbols.
     * @return True if empty.
     */
    bool empty() const { return symbols_.empty(); }

    /**
     * @brief Checks whether any symbol overload is registered under a name.
     * @param name Name to check.
     * @return True if at least one symbol is registered under that name.
     */
    bool contains(const std::string& name) const;

    /**
     * @brief Returns every symbol id registered under a name (all overloads).
     * @param name Name to look up.
     * @return Ids of matching symbols, or an empty deque if the name is unknown.
     */
    const std::deque<SymbolId> get(const std::string& name) const;

    /**
     * @brief Looks up the overload of a name whose type exactly matches a given type.
     * @param name Name to look up.
     * @param type Exact type to match.
     * @return The matching symbol, or empty if none matches.
     */
    optional_ref<const Symbol> get(const std::string& name, const type::Node& type) const;

    /**
     * @brief Looks up a symbol by its unique id.
     * @param id Id to look up.
     * @return The matching symbol. Behaviour is undefined if no symbol with this id is registered.
     */
    const Symbol& get(SymbolId id) const;

    /**
     * @brief Removes a symbol from both the id map and its name index.
     * @param id Id of the symbol to remove.
     */
    void remove(SymbolId id);

    /**
     * @brief Adds a new symbol into the registry, indexing it by both id and name.
     * @param symbol Symbol to take ownership of and insert.
     */
    void insert(std::unique_ptr<Symbol> symbol);

    /**
     * @brief Returns an iterator to the first symbol entry.
     * @return Iterator to the beginning of the id-keyed symbol map.
     */
    auto begin() { return symbols_.begin(); }

    /**
     * @brief Returns a const iterator to the first symbol entry.
     * @return Const iterator to the beginning of the id-keyed symbol map.
     */
    auto begin() const { return symbols_.begin(); }

    /**
     * @brief Returns an iterator past the last symbol entry.
     * @return Iterator to the end of the id-keyed symbol map.
     */
    auto end() { return symbols_.end(); }

    /**
     * @brief Returns a const iterator past the last symbol entry.
     * @return Const iterator to the end of the id-keyed symbol map.
     */
    auto end() const { return symbols_.end(); }
  };

  /**
   * @brief Bundles the inputs needed to construct and validate a new variable, argument, or function symbol.
   */
  struct VariableOptions {
    lexer::Token token; ///< Origin token providing the symbol's name.
    const type::Node& type; ///< Symbol's type.
    Category category; ///< Kind of symbol to create (variable, argument, function, ...).
    bool is_constant = false; ///< Whether the symbol is a constant (cannot be reassigned).
    optional_ref<ast::FunctionBaseNode> func_origin; ///< AST node backing a function-category symbol; required when `category == Function`.
  };

  /**
   * @brief Validates and inserts a new variable/argument/function symbol into a registry, handling name-shadowing and overload-collision rules.
   * @param registry Registry to insert into.
   * @param options Details of the symbol to create.
   * @param messages Optional message list to append a diagnostic to on failure; if omitted, failure is silent.
   * @return The newly assigned id, or nothing if the declaration collides with an existing overload.
   */
  // create and insert a variable into the given registry if permitted
  // return symbol's id, or nothing if error
  // if messages provided, append error to there, otherwise fail silently
  std::optional<SymbolId> create_variable(lang::symbol::Registry& registry, const VariableOptions& options, optional_ref<message::List> messages = std::nullopt);
}
