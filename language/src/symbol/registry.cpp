#include "registry.hpp"

bool lang::symbol::Registry::contains(const std::string& name) const {
  return names_.contains(name);
}

const std::deque<lang::symbol::SymbolId> lang::symbol::Registry::get(const std::string& name) const {
  if (auto it = names_.find(name); it != names_.end()) {
    return it->second;
  }
  return {};
}

lang::symbol::Symbol& lang::symbol::Registry::get_mut(SymbolId id) {
  return *symbols_.find(id)->second;
}

const lang::symbol::Symbol& lang::symbol::Registry::get(SymbolId id) const {
  return *symbols_.find(id)->second;
}

void lang::symbol::Registry::insert_name(const Symbol& symbol) {
  const std::string name = symbol.full_name();

  // record ID against the symbol's name
  if (auto it = names_.find(name); it != names_.end()) {
    it->second.push_front(symbol.id());
  } else {
    names_.insert({name, {symbol.id()}});
  }
}

void lang::symbol::Registry::insert(std::unique_ptr<Symbol> symbol) {
  const SymbolId id = symbol->id();
  insert_name(*symbol);
  symbols_.insert({id, std::move(symbol)});
}

void lang::symbol::Registry::remove(lang::symbol::SymbolId id) {
  if (!symbols_.contains(id)) return;
  const Symbol& symbol = *symbols_[id];

  // remove from the name map
  auto& symbols = names_[symbol.full_name()];
  std::erase_if(symbols, [id](SymbolId& _id) { return id == _id; });

  // erase from symbol map
  symbols_.erase(id);
}
