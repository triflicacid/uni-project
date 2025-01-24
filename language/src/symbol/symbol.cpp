#include "namespace.hpp"
#include "symbol.hpp"

static lang::symbol::SymbolId current_id = 0;

lang::symbol::Symbol::Symbol(lang::lexer::Token name) : token_(std::move(name)), id_(current_id++) {}

std::string lang::symbol::Symbol::full_name() const {
  // get components from parents
  std::deque<std::string> components;
  const Symbol* symbol = this;
  while (symbol) {
    components.push_front(symbol->name());
    if (auto& parent = symbol->parent(); parent.has_value()) {
      symbol = &parent.value().get();
    } else {
      break;
    }
  }

  // join components by seperator '.'
  std::stringstream full_name;
  full_name << components.front();
  components.pop_front();
  while (!components.empty()) {
    full_name << "." << components.front();
    components.pop_front();
  }
  return full_name.str();
}

const std::deque<std::unique_ptr<lang::symbol::Symbol>> *lang::symbol::Namespace::find(const std::string &name) const {
  if (auto it = members_.find(name); it != members_.end()) {
    return &it->second;
  }

  return nullptr;
}

void lang::symbol::Namespace::insert(std::unique_ptr<Symbol> symbol) {
  members_[symbol->token().image].push_back(std::move(symbol));
}
