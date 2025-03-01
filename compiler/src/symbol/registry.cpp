#include "registry.hpp"
#include "ast/types/function.hpp"
#include "function.hpp"

bool lang::symbol::Registry::contains(const std::string& name) const {
  return names_.contains(name);
}

const std::deque<lang::symbol::SymbolId> lang::symbol::Registry::get(const std::string& name) const {
  if (auto it = names_.find(name); it != names_.end()) {
    return it->second;
  }
  return {};
}

optional_ref<const lang::symbol::Symbol> lang::symbol::Registry::get(const std::string& name, const lang::ast::type::Node& type) const {
  if (auto it = names_.find(name); it != names_.end()) {
    for (SymbolId id : it->second) {
      const Symbol& symbol = get(id);
      if (symbol.type() == type) return symbol;
    }
  }
  return {};
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

std::optional<lang::symbol::SymbolId> lang::symbol::create_variable(lang::symbol::Registry& registry, const VariableOptions& options, optional_ref<message::List> messages) {
  // check if the symbol exists, we need additional guarding logic if it does
  const std::string name = options.token.image;
  auto& others = registry.get(name);
  if (!others.empty()) {
    // if we are a function, check if this overload already exists
    // otherwise, we are shadowing
    if (auto func_type = options.type.get_func()) {
      // iterate through each candidate, check if ID's are equal
      for (SymbolId id : others) {
        if (auto& symbol = registry.get(id); symbol.type() == *func_type) {
          // add messages to list if provided
          if (messages.has_value()) {
            auto msg = options.token.generate_message(message::Error);
            msg->get() << "unable to define overload - a function matching this signature already exists";
            messages->get().add(std::move(msg));

            msg = symbol.token().generate_message(message::Note);
            msg->get() << "signature " << name;
            func_type->print_code(msg->get());
            msg->get() << " previously defined here";
            messages->get().add(std::move(msg));
          }

          return {};
        }
      }
    } else {
      // if not empty, should only contain one element
      registry.remove(others.front());
    }
  }

  // create Symbol object & register in the symbol table
  std::unique_ptr<Symbol> symbol;
  if (options.category == Category::Function) {
    assert(options.type.get_func());
    assert(options.func_origin.has_value());
    symbol = std::make_unique<Function>(options.token, options.func_origin->get());
  } else {
    symbol = std::make_unique<symbol::Symbol>(options.token, options.category, options.type);
  }

  const SymbolId id = symbol->id();
  if (options.is_constant) symbol->make_constant();
  registry.insert(std::move(symbol));
  return id;
}
