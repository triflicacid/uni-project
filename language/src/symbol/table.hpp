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

namespace lang::ast {
  class FunctionBaseNode;
}

namespace lang::symbol {
  class SymbolTable {
    std::deque<std::unordered_map<std::string, std::unordered_set<SymbolId>>> scopes_; // variable stack, most recent = front, stores fully-qualified names
    std::unordered_map<SymbolId, memory::StorageLocation> storage_; // record where each symbol is physically stored, populated by ::locate()
    std::unordered_map<SymbolId, std::unique_ptr<Symbol>> symbols_;
    std::deque<std::reference_wrapper<const ast::FunctionBaseNode>> trace_; // track which function we are in, front = most recent
    memory::StackManager& stack_;

  public:
    SymbolTable(const SymbolTable&) = delete;
    SymbolTable(memory::StackManager& stack);

    // get a reference to the underlying StackManager
    memory::StackManager& stack() { return stack_; }

    // return symbol(s) with the given name
    const std::deque<std::reference_wrapper<Symbol>> find(const std::string& name) const;

    // return symbol with the given id
    const Symbol& get(SymbolId id) const;

    // insert symbol into the local scope
    // note, if `alloc=false`, call ::allocate() to physically allocate space for the symbol
    void insert(std::unique_ptr<Symbol> symbol, bool alloc = true);

    // insert contents of a registry - calls ::insert() on all symbols in registry
    // note, this moves symbols out of the registry, hence invalidates it
    void insert(Registry& registry, bool alloc = true);

    // allocate space for this symbol (e.g., push to stack, ...)
    // note, be careful not to allocate scope's in a different order
    void allocate(SymbolId symbol);

    // get the storage location of the given symbol
    // may be optional if the symbol (1) has not been allocated, or (2) has no physical width (e.g., a namespace)
    std::optional<std::reference_wrapper<const memory::StorageLocation>> locate(SymbolId symbol) const;

    // given a memory location, return asm argument referencing this location
    std::unique_ptr<assembly::BaseArg> resolve_location(const memory::StorageLocation& location) const;

    // assign given symbol to contents of the given register, inserting asm instructions in program
    // note: errors if symbol has no physical location
    void assign_symbol(SymbolId symbol, uint8_t reg) const;

    // create new lexical scope
    void push();

    // peek at the latest scope
    std::unordered_set<SymbolId> peek() const;

    // remove old lexical scope
    void pop();

    // record that we are in a new function
    void enter_function(const ast::FunctionBaseNode& f);

    // get the current function (if nothing, we are in global scope)
    std::optional<std::reference_wrapper<const ast::FunctionBaseNode>> current_function() const;

    // exit the last function
    void exit_function();
  };
}
