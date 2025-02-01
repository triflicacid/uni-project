#include <deque>
#include "namespace.hpp"
#include "symbol.hpp"
#include "value/symbol.hpp"


static lang::symbol::SymbolId current_id = 0;

lang::symbol::Symbol::Symbol(lang::lexer::Token name) : token_(std::move(name)), id_(current_id++) {}

lang::symbol::Symbol::Symbol(lexer::Token name, Category category)
  : token_(std::move(name)), category_(std::move(category)), id_(current_id++) {}

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

std::string lang::symbol::category_to_string(lang::symbol::Category category) {
  switch (category) {
    case Category::Ordinary:
      return "symbol";
    case Category::Argument:
      return "argument";
    case Category::Function:
      return "function";
    case Category::Namespace:
      return "namespace";
  }
}
