#include "future.hpp"
#include "context.hpp"
#include "message_helper.hpp"
#include "value.hpp"
#include "ast/types/unit.hpp"
#include "lvalue.hpp"
#include "rvalue.hpp"
#include "ast/types/int.hpp"
#include "operators/builtins.hpp"
#include "assembly/create.hpp"

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
  auto copy = std::make_unique<Value>();
  copy->type_ = type_;
  if (rvalue_) copy->rvalue(rvalue_->copy());
  if (lvalue_) copy->lvalue(lvalue_->copy());
  return copy;
}

bool lang::value::Value::materialise(lang::Context& ctx, const Location& origin) {
  return materialise(ctx, {{}, false, origin});
}

std::unique_ptr<lang::value::Value> lang::value::SymbolRef::copy() const {
  auto copy = std::make_unique<SymbolRef>(name_, overload_set_);
  if (is_rvalue()) copy->rvalue(rvalue().copy());
  if (is_lvalue()) copy->lvalue(lvalue().copy());
  return copy;
}

bool lang::value::SymbolRef::resolve(const message::MessageGenerator& source, optional_ref<message::List> messages, optional_ref<const ast::type::Node> type_hint) {
  auto candidates = overload_set_;

  // if no candidates, this symbol does not exist
  if (candidates.empty()) {
    if (messages.has_value()) messages->get().add(util::error_symbol_not_found(source, name_));
    return false;
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
      return false;
    }

    candidates = matches;
  }

  // if more than one candidate, we do not have sufficient info to choose
  if (candidates.size() > 1) {
    if (messages.has_value()) {
      messages->get().add(util::error_ambiguous_reference(source, name_));
      util::note_candidates(candidates, messages->get());
    }
    return false;
  }

  // fetch symbol
  symbol::Symbol& symbol = candidates.front().get();

  // create and set lvalue
  lvalue(std::make_unique<value::Symbol>(symbol));
  return true;
}

bool lang::value::SymbolRef::materialise(Context& ctx, const MaterialisationOptions& options) {
  // assume we have been resolved
  assert(is_lvalue() && lvalue().get_symbol());
  const int index = ctx.program.current().size();

  // get the symbol and its location
  const symbol::Symbol& symbol = lvalue().get_symbol()->get();
  auto location = ctx.symbols.locate(symbol.id());

  // if symbol has no location, that's tough
  if (!location) return true;

  // load location into a register, this is our rvalue
  memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(symbol);
  ref = ctx.reg_alloc_manager.guarantee_register(ref);
  rvalue(ref);

  // if needed, copy into target location
  bool materialised = false;
  if (options.target && options.target->get() != location->get() && options.copy_or_move && symbol.type().reference_as_ptr()) {
    ops::mem_copy(ctx, ref, *this, options.target->get().resolve());
    materialised = true;
  }

  // set line origins
  if (options.origin) {
    ctx.program.update_line_origins(options.origin.value(), index);
  }

  return materialised;
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

std::unique_ptr<lang::value::Value> lang::value::literal(const lang::memory::Literal& lit) {
  return std::make_unique<Literal>(lit);
}

std::unique_ptr<lang::value::Value> lang::value::contiguous_literal(const lang::ast::type::Node& type, ContiguousLiteral::Elements elements, bool is_global) {
  return std::make_unique<ContiguousLiteral>(type, std::move(elements), is_global);
}

std::unique_ptr<lang::value::Value> lang::value::literal(const lang::memory::Literal& lit, const lang::memory::Ref& ref) {
  auto value = literal(lit);
  value->rvalue(ref);
  return value;
}

std::unique_ptr<lang::value::LValue> lang::value::LValue::copy() const {
  return std::make_unique<LValue>(type_);
}

std::unique_ptr<lang::value::LValue> lang::value::Reference::copy() const {
  return std::make_unique<Reference>(type(), ref_);
}

std::unique_ptr<lang::value::RValue> lang::value::RValue::copy() const {
  return std::make_unique<RValue>(type_, ref_);
}

std::unique_ptr<lang::value::Value> lang::value::Literal::copy() const {
  auto copy = std::make_unique<Literal>(lit_);
  if (is_rvalue()) copy->rvalue(rvalue().copy());
  if (is_lvalue()) copy->lvalue(lvalue().copy());
  return copy;
}

bool lang::value::Literal::materialise(lang::Context& ctx, const lang::value::MaterialisationOptions& options) {
  const int index = ctx.program.current().size();

  // load and set rvalue
  memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(lit_);
  ref = ctx.reg_alloc_manager.guarantee_register(ref);
  rvalue(ref);

  if (options.target) {
    // move/copy into target
    auto target_arg = options.target->get().resolve();
    if (options.copy_or_move && lit_.type().reference_as_ptr()) {
      ops::mem_copy(ctx, ref, *this, std::move(target_arg));
    } else {
      assembly::create_store(ref.offset, std::move(target_arg), type().size(), ctx.program.current());
    }
  }

  // set line origins
  if (options.origin) {
    ctx.program.update_line_origins(options.origin.value(), index);
  }

  return true;
}

lang::value::ContiguousLiteral::ContiguousLiteral(const lang::ast::type::Node& type, Elements elements, bool is_global)
  : Value(type), elements_(std::move(elements)), global_(is_global) {}

std::unique_ptr<lang::value::Value> lang::value::ContiguousLiteral::copy() const {
  auto copy = std::make_unique<ContiguousLiteral>(type(), elements_, global_);
  if (is_rvalue()) copy->rvalue(rvalue().copy());
  if (is_lvalue()) copy->lvalue(lvalue().copy());
  return copy;
}

bool lang::value::ContiguousLiteral::materialise(lang::Context& ctx, const lang::value::MaterialisationOptions& options) {
  // location of contiguous block, note this always has a value
  memory::StorageLocation location = memory::StorageLocation::stack(0); // temporary

  // allocate space for us if we need
  if (options.target) {
    location = options.target->get();
  } else {
    optional_ref<std::stringstream> comment;

    if (global_) {
      // create block to reside in
      auto block = assembly::BasicBlock::labelled();
      location = memory::StorageLocation::global(*block);
      comment = block->comment();

      // insert into program
      auto& cursor = ctx.program.current();
      ctx.program.insert(assembly::Position::End, std::move(block));
      ctx.program.select(cursor);
    } else {
      // allocate space on the stack
      ctx.stack_manager.push(type().size());
      location = memory::StorageLocation::stack(ctx.stack_manager.offset());
      comment = ctx.program.current().back().comment();
    }

    // add comment
    comment->get() << "literal: ";
    type().print_code(comment->get());
  }

  // materialise our children
  int offset = 0;
  for (Value& element : elements_) {
    // attempt to materialise child
    if (!element.materialise(ctx, {location, options.copy_or_move, options.origin})) return false;

    // increase offset according to child's size
    // adjust location to child
    location = location + element.type().size();
  }

  // if we haven't got a target (temporary) and this is the outer block, load reference
  if (!options.target) {
    memory::Ref ref = ctx.reg_alloc_manager.insert({value::value(type())});
    ref = ctx.reg_alloc_manager.guarantee_register(ref);
    rvalue(ref);
  }

  return true;
}
