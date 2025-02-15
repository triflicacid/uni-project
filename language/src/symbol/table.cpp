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

optional_ref<lang::symbol::Symbol> lang::symbol::SymbolTable::find(const std::string& name, const lang::ast::type::Node& type) const {
  auto symbols = find(name);
  for (Symbol& symbol : symbols) {
    if (symbol.type() == type) return symbol;
  }
  return {};
}

void lang::symbol::SymbolTable::insert(std::unique_ptr<Symbol> symbol, bool alloc) {
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
    it->second.insert(id);
  } else {
    scopes_.front().insert({name, {id}});
  }

  // insert into cache
  symbols_.insert({id, std::move(symbol)});

  // allocate space for symbol if needs be
  if (alloc && category != Category::Function) allocate(id);
}

void lang::symbol::SymbolTable::allocate(lang::symbol::SymbolId id) {
  // do not allocate twice!
  assert(!storage_.contains(id));

  const symbol::Symbol& symbol = *symbols_.at(id);
  switch (symbol.category()) {
    case symbol::Category::Ordinary: // if size=0, immaterial
      if (size_t size = symbol.type().size()) {
        // are we global or no?
        if (in_global_scope()) {
          // create block for the variable to reside in
          auto block = assembly::BasicBlock::labelled("globl_" + std::to_string(id));
          block->comment() << "alloc " << symbol.full_name() << ": ";
          symbol.type().print_code(block->comment());

          // reserve space inside the block
          // TODO directly load data if possible
          block->add(assembly::Directive::space(symbol.type().size()));

          // register stored location
          storage_.insert({id, memory::StorageLocation::global(*block)});

          // insert block into the program
          stack_.program().insert(assembly::Position::After, std::move(block));
          stack_.program().select(assembly::Position::Previous);
        } else {
          // add symbol to stack iff size>0
          stack_.push(size);
          storage_.insert({id, memory::StorageLocation::stack(stack_.offset())});
          auto& comment = stack_.program().current().back().comment();
          comment << "alloc " << symbol.full_name() << ": ";
          symbol.type().print_code(comment);
        }
      }
      break;
    case symbol::Category::Argument: { // point to location in stack
      throw std::runtime_error("allocate: cannot allocate an argument");
//      // find which function this is an argument of
//      const ast::FunctionBaseNode* function = nullptr;
//      int trace_idx;
//      size_t offset; // offset from first arg in function
//      for (trace_idx = trace_.size() - 1; trace_idx >= 0; trace_idx--) {
//        offset = 0;
//        bool is_match = false;
//        // check if we are a parameter of this function
//        for (int j = 0; j < trace_[trace_idx].get().params(); j++) {
//          auto& param = trace_[trace_idx].get().param(j);
//          if (param.id() == id) {
//            is_match = true;
//            break;
//          }
//          offset += param.type().size();
//        }
//
//        if (is_match) {
//          function = &trace_[trace_idx].get();
//          break;
//        }
//      }
//
//      // if no match, then this is bad
//      if (!function) throw std::runtime_error("error when allocating argument - parent function cannot be found in trace");
//
//      // variable offset is from frame base + arg offset
//      offset += stack_.peek_frame(trace_idx);
//
//      // register stored location
//      storage_.insert({id, {offset - stack_.offset()}});
      break;
    }
    case symbol::Category::Function: { // bound to a block, function should already exist
      // create a new block for the function
      const std::string label = "func_" + std::to_string(id);
      auto block = assembly::BasicBlock::labelled(label);
      block->comment() << symbol.full_name();
      symbol.type().print_code(block->comment());

      // register stored location
      storage_.insert({id, memory::StorageLocation::global(*block)});

      // insert block into the program
      stack_.program().insert(assembly::Position::After, std::move(block));
      stack_.program().select(assembly::Position::Previous);
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

std::unique_ptr<lang::assembly::BaseArg>
lang::symbol::SymbolTable::resolve_location(const lang::memory::StorageLocation& location) const {
  switch (location.type) {
    case memory::StorageLocation::Block:
      return assembly::Arg::label(location.block);
    case memory::StorageLocation::Stack:
      return assembly::Arg::reg_indirect(constants::registers::fp, -location.offset); // remember to negate location from $fp as stack grows downwards
  }
}

void lang::symbol::SymbolTable::insert(Registry& registry, bool alloc) {
  for (auto& [symbolId, symbol] : registry.symbols_) {
    insert(std::move(symbol), alloc);
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
    // make sure to remove all symbols from storage_ and symbols_!
    for (auto& [name, symbols] : scopes_.front()) {
      for (SymbolId id: symbols) {
        symbols_.erase(id);
        storage_.erase(id);
      }
    }

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

void lang::symbol::SymbolTable::assign_symbol(lang::symbol::SymbolId symbol, uint8_t reg) const {
  const auto maybe_location = locate(symbol);
  assert(maybe_location.has_value());
  const memory::StorageLocation location = maybe_location.value().get();

  // store register into symbol (resolved location)
  auto argument = resolve_location(location);
  stack_.program().current().add(assembly::create_store(reg, std::move(argument)));
  auto& comment = stack_.program().current().back().comment();
  comment << get(symbol).full_name() << " = $" << constants::registers::to_string($reg(reg));
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
