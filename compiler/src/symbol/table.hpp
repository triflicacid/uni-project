#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include "symbol.hpp"
#include "memory/stack.hpp"
#include "memory/storage_location.hpp"
#include "assembly/arg.hpp"
#include "optional_ref.hpp"

namespace lang {
  namespace symbol {
    class Registry;
  }

  namespace ast {
    class FunctionBaseNode;
  }
}

/** @brief Symbol table and scoping: declared symbols, their storage locations, and lexical/function/namespace nesting. */
namespace lang::symbol {
  /**
   * @brief The compilation's global symbol table: owns every declared symbol, tracks lexical scoping, physical storage locations, and enclosing function/namespace nesting.
   *
   * Scopes are pushed/popped during the process phase, but symbols and their storage
   * are never removed on pop, since later resolve/generate_code phases may still need
   * to look them up by id.
   */
  class SymbolTable {
    std::deque<std::unordered_map<std::string, std::unordered_set<SymbolId>>> scopes_; ///< Scope stack of fully-qualified names to symbol ids; front is the most recent (innermost) scope.
    std::unordered_map<SymbolId, memory::StorageLocation> storage_; ///< Physical storage location of each allocated symbol, populated by @ref locate.
    std::unordered_map<SymbolId, std::unique_ptr<Symbol>> symbols_; ///< Every symbol ever inserted, keyed by id; never erased on scope pop.
    std::deque<std::reference_wrapper<const ast::FunctionBaseNode>> trace_; ///< Stack of enclosing functions being processed/generated; front is the most recent.
    std::deque<std::reference_wrapper<const Symbol>> path_; ///< Stack of enclosing namespace path segments; front is the most recent.
    memory::StackManager& stack_; ///< Stack manager used for stack-based symbol allocation.

  public:
    SymbolTable(const SymbolTable&) = delete;

    /**
     * @brief Constructs the table bound to a stack manager, with one initial (global) scope.
     * @param stack Stack manager used for stack-based symbol allocation.
     */
    SymbolTable(memory::StackManager& stack);

    /**
     * @brief Reports whether the table currently represents only the outermost (global) scope.
     * @return True if no additional scope is currently pushed.
     */
    // return if we are in the global scope or not
    bool in_global_scope() const { return scopes_.size() < 2; }

    /**
     * @brief Returns the underlying stack manager.
     * @return The stack manager.
     */
    // get a reference to the underlying StackManager
    memory::StackManager& stack() { return stack_; }

    /**
     * @brief Looks up every symbol overload visible under a name, searching from the innermost scope outward.
     * @param name Name to look up.
     * @return Matching symbols found in the first scope (searching innermost-out) that contains the name, or empty if none.
     */
    // return symbol(s) with the given name
    const std::deque<std::reference_wrapper<Symbol>> find(const std::string& name) const;

    /**
     * @brief Looks up the overload of a name matching an exact type.
     * @param name Name to look up.
     * @param type Exact type to match.
     * @return The matching symbol, or empty if none matches.
     */
    // return symbol with the given name and type
    optional_ref<Symbol> find(const std::string& name, const type::Node& type) const;

    /**
     * @brief Looks up a symbol by id, bypassing scope search.
     * @param id Id to look up.
     * @return The matching symbol.
     */
    // return symbol with the given id
    const Symbol& get(SymbolId id) const;

    /**
     * @brief Registers a new symbol into the current (innermost) scope, wiring up its namespace parent if applicable.
     * @param symbol Symbol to take ownership of and register.
     */
    // insert symbol into the local scope
    // note that this does not allocate space for ths symbol (doesn't emit any code)
    // also note that non-functional symbols are automatically shadowed
    void insert(std::unique_ptr<Symbol> symbol);

    /**
     * @brief Bulk-transfers every symbol owned by a registry into the table, then clears the registry.
     * @param registry Registry to move symbols out of.
     */
    // insert contents of a registry - calls ::insert() on all symbols in registry
    // note, this moves symbols out of the registry, hence invalidates it
    void insert(Registry& registry);

    /**
     * @brief Gives a previously-inserted symbol a concrete physical storage location, emitting whatever assembly scaffolding its category requires.
     * @param symbol Id of the symbol to allocate storage for.
     */
    // allocate space for this symbol (e.g., push to stack, ...)
    // note, be careful not to allocate scope's in a different order
    void allocate(SymbolId symbol);

    /**
     * @brief Directly assigns a symbol a caller-computed storage location, bypassing the category-driven allocation logic.
     * @param symbol Id of the symbol to assign storage for.
     * @param location Storage location to assign.
     */
    // tell symbol where it is located
    void allocate(SymbolId symbol, memory::StorageLocation location);

    /**
     * @brief Looks up where a symbol's storage currently lives.
     * @param symbol Id of the symbol to look up.
     * @return The storage location, or empty if the symbol has not been allocated or has no physical width.
     */
    // get the storage location of the given symbol
    // may be optional if the symbol (1) has not been allocated, or (2) has no physical width (e.g., a namespace)
    optional_ref<const memory::StorageLocation> locate(SymbolId symbol) const;

    /**
     * @brief Emits a store instruction copying a register's contents into a symbol's resolved physical storage.
     * @param symbol_id Id of the symbol to assign to.
     * @param reg Register holding the value to store.
     */
    // assign given symbol to contents of the given register, inserting asm instructions in program
    // note: errors if symbol has no physical location
    void assign_symbol(SymbolId symbol_id, uint8_t reg) const;

    /**
     * @brief Permanently removes a symbol from every scope's name map and from the id-to-owner cache.
     * @param symbol Id of the symbol to remove.
     */
    // remove the given symbol
    void erase(SymbolId symbol);

    /**
     * @brief Removes every symbol overload registered under a name in the current (innermost) scope.
     * @param name Name to remove.
     */
    // remove the given symbols from the local scope
    void erase(const std::string& name);

    /**
     * @brief Opens a new, empty lexical scope.
     */
    // create new lexical scope
    void push();

    /**
     * @brief Collects every symbol id currently bound in just the innermost scope.
     * @return Set of ids bound directly in the innermost scope.
     */
    // peek at the latest scope
    std::unordered_set<SymbolId> peek() const;

    /**
     * @brief Closes the innermost lexical scope, without removing any underlying symbol data.
     */
    // remove old lexical scope
    void pop();

    /**
     * @brief Records that code generation has descended into a new function body.
     * @param f Function node being entered.
     */
    // record that we are in a new function
    void enter_function(const ast::FunctionBaseNode& f);

    /**
     * @brief Reports which function, if any, is currently being processed or generated.
     * @return The innermost enclosing function, or empty if at global scope.
     */
    // get the current function (if nothing, we are in global scope)
    std::optional<std::reference_wrapper<const ast::FunctionBaseNode>> current_function() const;

    /**
     * @brief Records that code generation has left the current function body.
     */
    // exit the last function
    void exit_function();

    /**
     * @brief Records that processing has descended into a named container (namespace), extending the active qualification path.
     * @param id Id of the namespace symbol being entered.
     */
    // record that we are in a new named container
    void push_path(SymbolId id);

    /**
     * @brief Looks at the nth most recently pushed path entry without removing it.
     * @param n Depth to peek at, where 0 is the innermost/most recent entry.
     * @return The path entry at that depth, or empty if there are fewer than n+1 entries.
     */
    // get `n`th most recent path item (default n = 0 = most recent)
    optional_ref<const Symbol> peek_path(unsigned int n = 0);

    /**
     * @brief Records that processing has left the innermost currently-open named container.
     */
    // exit the current named container
    void pop_path();

    /**
     * @brief Builds the fully dot-qualified name of the current namespace path.
     * @return The qualified path name, or an empty string if the path is empty.
     */
    // generate full path name
    std::string path_name() const;

    /**
     * @brief Builds the fully dot-qualified name of the current namespace path with a trailing name appended.
     * @param name Name to append after the qualified path.
     * @return The qualified name.
     */
    // generate full path name with `name` appended on the end
    std::string path_name(const std::string& name) const;
  };
}
