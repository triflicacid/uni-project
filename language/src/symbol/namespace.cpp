#include "namespace.hpp"


std::deque<std::unique_ptr<lang::symbol::Symbol>> *lang::symbol::Namespace::lookup(const std::string &name) {
  if (auto it = members_.find(name); it != members_.end()) {
    return &it->second;
  }

  return nullptr;
}

void lang::symbol::Namespace::insert(std::unique_ptr<Symbol> symbol) {
  members_[symbol->name().image].push_back(std::move(symbol));
}
