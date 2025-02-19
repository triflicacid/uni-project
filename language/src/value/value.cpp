#include "symbol.hpp"
#include "context.hpp"
#include "message_helper.hpp"
#include "value.hpp"
#include "ast/types/unit.hpp"
#include "lvalue.hpp"
#include "rvalue.hpp"


lang::value::Value::Value() : type_(ast::type::unit) {}

lang::value::Value::Value(const ast::type::Node& type) : type_(type) {}

lang::value::LValue& lang::value::Value::lvalue() const {
  assert(is_lvalue());
  return *lvalue_;
}

void lang::value::Value::lvalue(std::unique_ptr<LValue> v) {
  type_ = v->type();
  lvalue_ = std::move(v);
}

lang::value::RValue& lang::value::Value::rvalue() const {
  assert(is_rvalue());
  return *rvalue_;
}

void lang::value::Value::rvalue(std::unique_ptr<RValue> v) {
  type_ = v->type();
  rvalue_ = std::move(v);
}

void lang::value::Value::rvalue(const lang::memory::Ref& ref) {
  rvalue_ = std::make_unique<value::RValue>(type_, ref);
}

void lang::value::Value::lvalue(const lang::memory::Ref& ref) {
  lvalue_ = std::make_unique<value::Reference>(type_, ref);
}

void lang::value::Value::lvalue(const lang::symbol::Symbol& symbol) {
  lvalue_ = std::make_unique<value::Symbol>(symbol);
}

std::unique_ptr<lang::value::Value> lang::value::Value::copy() const {
  auto copy = std::unique_ptr<Value>();
  if (rvalue_) copy->rvalue(rvalue_->copy());
  if (lvalue_) copy->lvalue(lvalue_->copy());
  return copy;
}

std::unique_ptr<lang::value::Symbol> lang::value::SymbolRef::resolve(const message::MessageGenerator& source, optional_ref<message::List> messages, optional_ref<const ast::type::Node> type_hint) const {
  auto candidates = overload_set_;

  // if no candidates, this symbol does not exist
  if (candidates.empty()) {
    if (messages.has_value()) messages->get().add(util::error_symbol_not_found(source, name_));
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
      if (messages.has_value()) {
        messages->get().add(util::error_cannot_match_type_hint(source, name_, type_hint->get()));
        util::note_candidates(candidates, messages->get());
      }
      return nullptr;
    }

    candidates = matches;
  }

  // if more than one candidate, we do not have sufficient info to choose
  if (candidates.size() > 1) {
    if (messages.has_value()) {
      messages->get().add(util::error_ambiguous_reference(source, name_));
      util::note_candidates(candidates, messages->get());
    }
    return nullptr;
  }

  // fetch symbol
  symbol::Symbol& symbol = candidates.front().get();

  // create Value instance
  return std::make_unique<value::Symbol>(symbol);
}

lang::value::Symbol::Symbol(const symbol::Symbol& symbol) : LValue(symbol.type()), symbol_(symbol) {}

std::unique_ptr<lang::value::LValue> lang::value::Symbol::copy() const {
  return std::make_unique<Symbol>(symbol_);
}

std::unique_ptr<lang::value::SymbolRef> lang::value::symbol_ref(const std::string name, const symbol::SymbolTable& symbols) {
  return std::make_unique<SymbolRef>(name, symbols.find(name));
}

std::unique_ptr<lang::value::Value> lang::value::value(optional_ref<const lang::ast::type::Node> type) {
  if (type.has_value()) {
    return std::make_unique<Value>(type->get());
  } else {
    return std::make_unique<Value>();
  }
}

std::unique_ptr<lang::value::Value> lang::value::rvalue(const lang::ast::type::Node& type, const lang::memory::Ref& ref) {
  auto value = std::make_unique<Value>(type);
  value->rvalue(ref);
  return value;
}

std::unique_ptr<lang::value::Value> lang::value::unit_value() {
  return rvalue(ast::type::unit, memory::Ref::reg(-1));
}

const std::unique_ptr<lang::value::Value> lang::value::unit_value_instance = unit_value();

std::unique_ptr<lang::value::LValue> lang::value::LValue::copy() const {
  return std::make_unique<LValue>(type_);
}

std::unique_ptr<lang::value::LValue> lang::value::Reference::copy() const {
  return std::make_unique<Reference>(type(), ref_);
}

std::unique_ptr<lang::value::RValue> lang::value::RValue::copy() const {
  return std::make_unique<RValue>(type_, ref_);
}

std::unique_ptr<lang::value::RValue> lang::value::Literal::copy() const {
  return std::make_unique<Literal>(lit_, ref());
}
