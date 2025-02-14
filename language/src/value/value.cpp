#include "symbol.hpp"
#include "context.hpp"
#include "message_helper.hpp"
#include "value.hpp"
#include "ast/types/unit.hpp"

lang::value::Value::Value() : type_(ast::type::unit) {}

lang::value::Value::Value(bool future_lvalue, bool future_rvalue) : future_lvalue_(future_lvalue), future_rvalue_(future_rvalue), type_(ast::type::unit) {}

lang::value::Value::Value(const ast::type::Node& type, bool future_lvalue, bool future_rvalue) : future_lvalue_(future_lvalue), future_rvalue_(future_rvalue), type_(type) {}

lang::value::LValue& lang::value::Value::lvalue() const {
  assert(is_lvalue());
  return *lvalue_;
}

void lang::value::Value::lvalue(std::unique_ptr<LValue> v) {
  future_lvalue_ = false;
  type_ = v->type();
  lvalue_ = std::move(v);
}

lang::value::RValue& lang::value::Value::rvalue() const {
  assert(is_rvalue());
  return *rvalue_;
}

void lang::value::Value::rvalue(std::unique_ptr<RValue> v) {
  future_rvalue_ = false;
  type_ = v->type();
  rvalue_ = std::move(v);
}

void lang::value::Value::rvalue(const lang::memory::Ref& ref) {
  future_rvalue_ = false;
  rvalue_ = std::make_unique<value::RValue>(type_, ref);
}

void lang::value::Value::lvalue(const lang::memory::Ref& ref) {
  future_lvalue_ = false;
  lvalue_ = std::make_unique<value::Reference>(type_, ref);
}

std::unique_ptr<lang::value::Symbol> lang::value::SymbolRef::resolve(lang::Context& ctx, const message::MessageGenerator& source, bool generate_messages, optional_ref<const ast::type::Node> type_hint) const {
  auto candidates = ctx.symbols.find(name_);

  // if no candidates, this symbol does not exist
  if (candidates.empty()) {
    if (generate_messages) ctx.messages.add(util::error_symbol_not_found(source, name_));
    return nullptr;
  }

  // if more than one candidate, match against type hint
  if (type_hint.has_value() && candidates.size() > 1) {
    // check for matches
    std::deque<std::reference_wrapper<symbol::Symbol>> matches;
    for (symbol::Symbol& candidate : candidates) {
      if (candidate.type() == *type_hint)
        matches.push_back(candidate);
    }

    // if no matches, complain
    if (matches.empty()) {
      if (generate_messages) {
        ctx.messages.add(util::error_cannot_match_type_hint(source, name_, type_hint->get()));
        util::note_candidates(candidates, ctx.messages);
      }
      return nullptr;
    }

    candidates = matches;
  }

  // if more than one candidate, we do not have sufficient info to choose
  if (candidates.size() > 1) {
    if (generate_messages) {
      ctx.messages.add(util::error_insufficient_info_to_resolve_symbol(source, name_));
      util::note_candidates(candidates, ctx.messages);
    }
    return nullptr;
  }

  // fetch symbol
  symbol::Symbol& symbol = candidates.front().get();

  // create Value instance (symbol so lvalue)
  auto location = ctx.symbols.locate(symbol.id());
  return std::make_unique<value::Symbol>(symbol);
}

std::unique_ptr<lang::value::Value> lang::value::rvalue() {
  return std::make_unique<Value>(false, true);
}

std::unique_ptr<lang::value::Value> lang::value::rvalue(const ast::type::Node& type) {
  return std::make_unique<Value>(type, false, true);
}

std::unique_ptr<lang::value::Value> lang::value::rvalue(const lang::ast::type::Node& type, const lang::memory::Ref& ref) {
  auto value = rvalue();
  value->rvalue(std::make_unique<RValue>(type, ref));
  return value;
}

std::unique_ptr<lang::value::Value> lang::value::lvalue() {
  return std::make_unique<Value>(true, false);
}

std::unique_ptr<lang::value::Value> lang::value::lvalue(const ast::type::Node& type) {
  return std::make_unique<Value>(type, true, false);
}

std::unique_ptr<lang::value::Value> lang::value::rlvalue() {
  return std::make_unique<Value>(true, true);
}

std::unique_ptr<lang::value::Value> lang::value::rlvalue(const ast::type::Node& type) {
  return std::make_unique<Value>(type, true, true);
}

lang::value::Symbol::Symbol(const symbol::Symbol& symbol) : LValue(symbol.type()), symbol_(symbol) {}

std::unique_ptr<lang::value::SymbolRef> lang::value::symbol_ref(const std::string name) {
  return std::make_unique<SymbolRef>(name);
}

//const lang::value::Value lang::value::unit_value(ast::type::unit, false, false);
const std::unique_ptr<lang::value::Value> lang::value::unit_value = rvalue(ast::type::unit, memory::Ref::reg(-1));
