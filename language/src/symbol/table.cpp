#include "table.hpp"

lang::symbol::SymbolTable::SymbolTable(memory::StackManager& stack) : stack_(stack) {
  scopes_.emplace_back();
}

const std::deque<std::unique_ptr<lang::symbol::Symbol>>* lang::symbol::SymbolTable::find(const std::string& name) const {
  for (auto& map : scopes_) {
    if (auto it = map.find(name); it != map.end()) {
      return &it->second;
    }
  }

  return nullptr;
}

void lang::symbol::SymbolTable::_insert(std::unique_ptr<Symbol> symbol) {
  const std::string& name = symbol->name().image;

  if (auto it = scopes_.back().find(name); it != scopes_.back().end()) {
    it->second.push_back(std::move(symbol));
  } else {
    std::deque<std::unique_ptr<Symbol>> symbols;
    symbols.push_back(std::move(symbol));
    scopes_.back().insert({name, std::move(symbols)});
  }
}

void lang::symbol::SymbolTable::insert(std::unique_ptr<Symbol> symbol) {
  if (size_t size = symbol->type().size(); size > 0) {
    stack_.push(size);
    storage_.insert({symbol->id(), {stack_.offset()}});
    stack_.program().current().back().comment() << "allocate space for symbol " << symbol->name().image;
  }

  _insert(std::move(symbol));
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
