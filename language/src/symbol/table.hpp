#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include <stack>
#include "symbol.hpp"
#include "memory/stack.hpp"
#include "memory/storage_location.hpp"

namespace lang::ast {
  class FunctionNode;
}

namespace lang::symbol {
  class SymbolTable {
    std::deque<std::unordered_map<std::string, std::deque<std::unique_ptr<Symbol>>>> scopes_; // variable stack, [0] refers to the global space (note, uses fully-qualified names)
    std::unordered_map<SymbolId, memory::StorageLocation> storage_;
    std::stack<std::reference_wrapper<const ast::FunctionNode>> trace_; // track which function we are in
    memory::StackManager& stack_;

    // insert symbol into the local scope
    void _insert(std::unique_ptr<Symbol> symbol);

  public:
    SymbolTable(const SymbolTable&) = delete;
    SymbolTable(memory::StackManager& stack);

    // get a reference to the underlying StackManager
    memory::StackManager& stack() { return stack_; }

    // return symbol with the given name
    const std::deque<std::reference_wrapper<Symbol>> find(const std::string& name) const;

    // insert symbol into the local scope
    void insert_local(std::unique_ptr<Symbol> symbol);

    // insert symbol
    // if global, bind to a BasicBlock
    void insert(std::unique_ptr<Symbol> symbol);

    // insert() but bind to a BasicBlock too, used for globals & functions
    void insert(std::unique_ptr<Symbol> symbol, assembly::BasicBlock& block);

    // insert contents of a registry
    // note, this moves symbols out of the registry, hence invalidates it
    void insert(Registry& registry);

    // get the storage location of the given symbol
    const memory::StorageLocation& locate(SymbolId symbol) const;

    // create new lexical scope
    void push();

    // remove old lexical scope
    void pop();

    // record that we are in a new function
    void enter_function(const ast::FunctionNode& f);

    // get the current function (if nothing, we are in global scope)
    std::optional<std::reference_wrapper<const ast::FunctionNode>> current_function() const;

    // exit the last function
    void exit_function();
  };
}
