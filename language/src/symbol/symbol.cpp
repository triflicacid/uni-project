#include <deque>
#include "symbol.hpp"
#include "value/symbol.hpp"
#include "ast/types/namespace.hpp"

static lang::symbol::SymbolId current_id = 0;

lang::symbol::Symbol::Symbol(lang::lexer::Token name, const ast::type::Node& type) : token_(std::move(name)), id_(current_id++), type_(type) {}

lang::symbol::Symbol::Symbol(lexer::Token name, Category category, const ast::type::Node& type)
  : token_(std::move(name)), category_(std::move(category)), id_(current_id++), type_(type) {}

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

std::unique_ptr<lang::symbol::Symbol> lang::symbol::create_namespace(const lexer::Token& name) {
  return std::make_unique<Symbol>(name, Category::Namespace, ast::type::name_space);
}
