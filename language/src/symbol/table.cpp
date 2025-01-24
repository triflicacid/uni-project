#include "table.hpp"
#include "assembly/directive.hpp"
#include "registry.hpp"

lang::symbol::SymbolTable::SymbolTable(memory::StackManager& stack) : stack_(stack) {
  scopes_.emplace_back();
}

const std::deque<std::reference_wrapper<lang::symbol::Symbol>> lang::symbol::SymbolTable::find(const std::string& name) const {
  for (auto& map : scopes_) {
    if (auto it = map.find(name); it != map.end()) {
      // return list of references
      std::deque<std::reference_wrapper<lang::symbol::Symbol>> symbols;
      for (auto& symbol : it->second) {
        symbols.push_back(*symbol);
      }
      return symbols;
    }
  }

  return {};
}

void lang::symbol::SymbolTable::_insert(std::unique_ptr<Symbol> symbol) {
  const std::string& name = symbol->token().image;

  if (auto it = scopes_.back().find(name); it != scopes_.back().end()) {
    it->second.push_back(std::move(symbol));
  } else {
    std::deque<std::unique_ptr<Symbol>> symbols;
    symbols.push_back(std::move(symbol));
    scopes_.back().insert({name, std::move(symbols)});
  }
}

void lang::symbol::SymbolTable::insert_local(std::unique_ptr<Symbol> symbol) {
  if (size_t size = symbol->type().size(); size > 0) {
    stack_.push(size);
    storage_.insert({symbol->id(), {stack_.offset()}});
    auto &comment = stack_.program().current().back().comment() << symbol->full_name() << ": ";
    symbol->type().print_code(comment);
  }

  _insert(std::move(symbol));
}

void lang::symbol::SymbolTable::insert(std::unique_ptr<Symbol> symbol) {
  // in function <=> local scope => stack variable
  if (!trace_.empty()) {
    insert_local(std::move(symbol));
    return;
  }

  // create block for the variable to reside in
  auto block = assembly::BasicBlock::labelled("globl_" + std::to_string(symbol->id()));
  block->comment() << "global " << symbol->full_name() << ": ";
  symbol->type().print_code(block->comment());

  // reserve space inside the block
  // TODO directly load data if possible
  block->add(assembly::Directive::space(symbol->type().size()));

  // register symbol against this block
  insert(std::move(symbol), *block);

  // insert block into the program
  stack_.program().insert(assembly::Position::After, std::move(block));
  stack_.program().select(assembly::Position::Previous);
}

void lang::symbol::SymbolTable::insert(std::unique_ptr<Symbol> symbol, lang::assembly::BasicBlock& block) {
  storage_.insert({symbol->id(), {block}});
  _insert(std::move(symbol));
}

const lang::memory::StorageLocation& lang::symbol::SymbolTable::locate(lang::symbol::SymbolId symbol) const {
  if (auto it = storage_.find(symbol); it != storage_.end())
    return it->second;
  throw std::runtime_error("locate: symbol ID has no location");
}

void lang::symbol::SymbolTable::insert(Registry& registry) {
  for (auto& [symbolId, symbol] : registry.symbols_) {
    insert_local(std::move(symbol));
  }

  // clear registry to minimise possibility of misuse
  registry.symbols_.clear();
  registry.names_.clear();
}

void lang::symbol::SymbolTable::push() {
  scopes_.emplace_back();
  stack_.push_frame();
}

void lang::symbol::SymbolTable::pop() {
  if (!scopes_.empty()) {
    scopes_.pop_back();
    stack_.pop_frame();
  }
  if (scopes_.empty()) push();
}

void lang::symbol::SymbolTable::enter_function(const lang::ast::FunctionNode& f) {
  trace_.push(f);
}

std::optional<std::reference_wrapper<const lang::ast::FunctionNode>> lang::symbol::SymbolTable::current_function() const {
  if (trace_.empty()) return {};
  return trace_.top();
}

void lang::symbol::SymbolTable::exit_function() {
  if (!trace_.empty()) trace_.pop();
}
