#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include "symbol.hpp"
#include "assembly/basic_block.hpp"
#include "memory/stack.hpp"
#include "memory/storage_location.hpp"

namespace lang::symbol {
  class SymbolTable {
    std::deque<std::unordered_map<std::string, std::deque<std::unique_ptr<Symbol>>>> scopes_; // variable stack, [0] refers to the global space
    std::unordered_map<SymbolId, memory::StorageLocation> storage_;
    memory::StackManager& stack_;

    // insert symbol into the local scope
    void _insert(std::unique_ptr<Symbol> symbol);

  public:
    SymbolTable(const SymbolTable&) = delete;
    SymbolTable(memory::StackManager& stack);

    // get a reference to the underlying StackManager
    const memory::StackManager& stack() const { return stack_; }

    // return symbol with the given name
    const std::deque<std::unique_ptr<Symbol>>* find(const std::string& name) const;

    // insert symbol into the local scope
    void insert(std::unique_ptr<Symbol> symbol);

    // insert() but bind to a BasicBlock too, used for globals & functions
    void insert(std::unique_ptr<Symbol> symbol, assembly::BasicBlock& block);

    // get the storage location of the given symbol
    const memory::StorageLocation& locate(SymbolId symbol) const;

    // create new lexical scope
    void push();

    // remove old lexical scope
    void pop();
  };
}
