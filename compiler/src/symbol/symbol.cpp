#include <deque>
#include "symbol.hpp"
#include "types/namespace.hpp"
#include "context.hpp"

static lang::symbol::SymbolId current_id = 0;

lang::symbol::Symbol::Symbol(lang::lexer::Token name, const type::Node& type) : token_(std::move(name)), id_(current_id++), type_(type) {}

lang::symbol::Symbol::Symbol(lexer::Token name, Category category, const type::Node& type)
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

bool lang::symbol::Symbol::define(Context& ctx) const {
  // check if we're defined already
  auto maybe_loc = ctx.symbols.locate(id_);

  if (!maybe_loc.has_value()) {
    // TODO move SymbolDeclarationNode here, only define when needed rather than in ::generate_code
    // if not, allocate
    ctx.symbols.allocate(id_);
  }

  return true;
}

std::string lang::symbol::category_to_string(lang::symbol::Category category) {
  switch (category) {
    case Category::Global:
      return "global";
    case Category::StackBased:
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
  auto ns = std::make_unique<Symbol>(name, Category::Namespace, type::name_space);
  ns->make_constant();
  return ns;
}
