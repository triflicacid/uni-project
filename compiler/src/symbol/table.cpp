#include <cassert>
#include "table.hpp"
#include "assembly/directive.hpp"
#include "registry.hpp"
#include "ast/function/function_base.hpp"
#include "ast/symbol_declaration.hpp"
#include "assembly/create.hpp"
#include "function.hpp"

lang::symbol::SymbolTable::SymbolTable(memory::StackManager& stack) : stack_(stack) {
  scopes_.emplace_front();
}

const std::deque<std::reference_wrapper<lang::symbol::Symbol>> lang::symbol::SymbolTable::find(const std::string& name) const {
  for (auto& map : scopes_) {
    if (auto it = map.find(name); it != map.end()) {
      // return list of references
      std::deque<std::reference_wrapper<lang::symbol::Symbol>> symbols;
      for (SymbolId id : it->second) {
        symbols.push_back(*symbols_.at(id));
      }
      return symbols;
    }
  }

  return {};
}

optional_ref<lang::symbol::Symbol> lang::symbol::SymbolTable::find(const std::string& name, const lang::type::Node& type) const {
  auto symbols = find(name);
  for (Symbol& symbol : symbols) {
    if (symbol.type() == type) return symbol;
  }
  return {};
}

void lang::symbol::SymbolTable::insert(std::unique_ptr<Symbol> symbol) {
  // look at path_, add parent UNLESS argument
  const Category category = symbol->category();
  if (category != Category::Argument) {
    if (auto parent = peek_path()) {
      symbol->set_parent(*parent);
    }
  }

  const std::string& name = symbol->full_name();
  const SymbolId id = symbol->id();

  // insert into scope data structure
  if (auto it = scopes_.front().find(name); it != scopes_.front().end()) {
    // shadow if symbol's type is non-functional
    // assume checks have been done beforehand to check this is OK
    if (symbol->type().get_func()) {
      it->second.insert(id);
    } else {
      it->second = {id};
    }
  } else {
    scopes_.front().insert({name, {id}});
  }

  // insert into cache
  symbols_.insert({id, std::move(symbol)});
}

void lang::symbol::SymbolTable::allocate(lang::symbol::SymbolId id) {
  assert(symbols_.contains(id));
  assert(!storage_.contains(id)); // do not allocate twice!
  assembly::Program& program = stack_.program();

  // get the symbol
  const symbol::Symbol& symbol = *symbols_.at(id);

  // if size==0, do nothing
  if (symbol.type().size() == 0) return;

  switch (symbol.category()) {
    case Category::Global: { // if size=0, immaterial
      // create block for the variable to reside in
      auto block = assembly::BasicBlock::labelled("globl_" + std::to_string(id));
      block->comment() << "alloc " << symbol.full_name() << ": ";
      symbol.type().print_code(block->comment());

      // reserve space inside the block
      // TODO directly load data if possible
      block->add(assembly::Directive::space(symbol.type().size()));

      // set source location
      if (auto loc = program.location()) {
        block->origin(loc->get());
        block->back().origin(loc->get());
      }

      // register stored location
      storage_.insert({id, memory::StorageLocation::global(*block)});

      // insert block into the program
      const auto& cursor = program.current();
      program.insert(assembly::Position::End, std::move(block));
      program.select(cursor);

      break;
    }
    case symbol::Category::StackBased: {
      // add symbol to stack, adjust fp
      stack_.push(symbol.type().size());
      storage_.insert({id, memory::StorageLocation::stack(stack_.offset())});

      auto& comment = program.current().back().comment();
      comment << "alloc " << symbol.full_name() << ": ";
      symbol.type().print_code(comment);

      // set source location
      if (auto loc = program.location()) program.current().back().origin(loc->get());

      break;
    }
    case symbol::Category::Argument: { // point to location in stack
      throw std::runtime_error("allocate: cannot allocate an argument");
    }
    case symbol::Category::Function: { // bound to a block, function should already exist
      // create a new block for the function
      const std::string label = "func_" + std::to_string(id);
      auto block = assembly::BasicBlock::labelled(label);
      block->comment() << symbol.full_name();
      symbol.type().print_code(block->comment());

      // set source location
      if (auto loc = program.location()) block->origin(loc->get());

      // register stored location
      storage_.insert({id, memory::StorageLocation::global(*block)});

      // insert block into the program
      const auto& cursor = program.current();
      program.insert(assembly::Position::End, std::move(block));
      program.select(cursor);
      break;
    }
    case symbol::Category::Namespace:
      break;
  }
}

void lang::symbol::SymbolTable::allocate(lang::symbol::SymbolId id, lang::memory::StorageLocation location) {
  // do not allocate twice!
  assert(!storage_.contains(id));

  storage_.insert({id, std::move(location)});
}

optional_ref<const lang::memory::StorageLocation> lang::symbol::SymbolTable::locate(lang::symbol::SymbolId id) const {
  if (auto it = storage_.find(id); it != storage_.end())
    return it->second;
  return {};
}

void lang::symbol::SymbolTable::insert(Registry& registry) {
  for (auto& [symbolId, symbol] : registry.symbols_) {
    insert(std::move(symbol));
  }

  // clear registry to minimise possibility of misuse
  registry.symbols_.clear();
  registry.names_.clear();
}

void lang::symbol::SymbolTable::push() {
  scopes_.emplace_front();
}

void lang::symbol::SymbolTable::pop() {
  if (!scopes_.empty()) {
    // note, DO NOT remove symbols from storage nor dictionary - they may be referenced in the future
    // push/pop/insert are used during ::process, but may be referenced in ::resolve and ::generate_code
    scopes_.pop_front();
  }
  if (scopes_.empty()) push();
}

void lang::symbol::SymbolTable::enter_function(const lang::ast::FunctionBaseNode& f) {
  trace_.push_front(f);
}

std::optional<std::reference_wrapper<const lang::ast::FunctionBaseNode>> lang::symbol::SymbolTable::current_function() const {
  if (trace_.empty()) return {};
  return trace_.front();
}

void lang::symbol::SymbolTable::exit_function() {
  if (!trace_.empty()) trace_.pop_front();
}

const lang::symbol::Symbol& lang::symbol::SymbolTable::get(lang::symbol::SymbolId id) const {
  return *symbols_.at(id);
}

std::unordered_set<lang::symbol::SymbolId> lang::symbol::SymbolTable::peek() const {
  // form 'local' symbol set by merging all symbols
  std::unordered_set<SymbolId> local;
  for (auto& [_, symbols] : scopes_.front()) {
    local.insert(symbols.begin(), symbols.end());
  }
  return local;
}

void lang::symbol::SymbolTable::assign_symbol(lang::symbol::SymbolId symbol_id, uint8_t reg) const {
  // get the symbol's location
  const auto maybe_location = locate(symbol_id);
  assert(maybe_location.has_value());
  const memory::StorageLocation& location = maybe_location.value().get();

  // get the actual symbol, exist if size==0
  const symbol::Symbol& symbol = get(symbol_id);
  if (symbol.type().size() == 0) return;

  // store register into symbol (resolved location)
  const int idx = stack_.program().current().size();
  assembly::create_store(reg, location.resolve(), symbol.type().size(), stack_.program().current());

  auto& comment = stack_.program().current()[idx].comment();
  comment << get(symbol_id).full_name() << " = $" << constants::registers::to_string($reg(reg));
}

void lang::symbol::SymbolTable::push_path(lang::symbol::SymbolId id) {
  path_.push_front(get(id));
}

void lang::symbol::SymbolTable::pop_path() {
  if (!path_.empty()) path_.pop_front();
}

optional_ref<const lang::symbol::Symbol> lang::symbol::SymbolTable::peek_path(unsigned int n) {
  if (n < path_.size()) return path_[n];
  return {};
}

std::string lang::symbol::SymbolTable::path_name() const {
  std::stringstream stream;
  for (int i = path_.size() - 1; i >= 0; i--) {
    stream << path_[i].get().name();
    if (i > 0) stream << ".";
  }
  return stream.str();
}

std::string lang::symbol::SymbolTable::path_name(const std::string& name) const {
  if (path_.empty()) return name;

  std::stringstream stream;
  for (int i = path_.size() - 1; i >= 0; i--) {
    stream << path_[i].get().name() << ".";
  }
  stream << name;
  return stream.str();
}

void lang::symbol::SymbolTable::erase(lang::symbol::SymbolId id) {
  if (!symbols_.contains(id)) return;
  auto& symbol = symbols_.at(id);

  // remove from scopes
  for (auto& scope : scopes_) {
    scope.at(symbol->name()).erase(id);
  }

  // remove from symbols_
  symbols_.erase(id);
}

void lang::symbol::SymbolTable::erase(const std::string& name) {
  if (scopes_.empty()) return;
  auto& scope = scopes_.front();
  if (!scope.contains(name)) return;

  // remove all IDs from dictionary
  for (SymbolId id : scope.at(name)) {
    symbols_.erase(id);
  }

  // remove entry in scope
  scope.erase(name);
}
