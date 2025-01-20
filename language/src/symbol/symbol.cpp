#include "namespace.hpp"
#include "symbol.hpp"

static lang::symbol::SymbolId current_id = 0;

lang::symbol::Symbol::Symbol(lang::lexer::Token name) : name_(std::move(name)), id_(current_id++) {}

const std::deque<std::unique_ptr<lang::symbol::Symbol>> *lang::symbol::Namespace::find(const std::string &name) const {
  if (auto it = members_.find(name); it != members_.end()) {
    return &it->second;
  }

  return nullptr;
}

void lang::symbol::Namespace::insert(std::unique_ptr<Symbol> symbol) {
  members_[symbol->name().image].push_back(std::move(symbol));
}
