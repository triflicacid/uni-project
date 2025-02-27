#pragma once

#include "value.hpp"
#include "symbol/table.hpp"
#include "ast/types/unit.hpp"

namespace lang::value {
  // represents a symbol name (reference)
  // we are not computable, either - if we were, return a Symbol
  // therefore, this is generally used when we have multiple options to pick from
  class SymbolRef : public Value {
    std::string name_;
    std::deque<std::reference_wrapper<symbol::Symbol>> overload_set_;

  public:
    SymbolRef(std::string name, std::deque<std::reference_wrapper<symbol::Symbol>> overload_set) : name_(std::move(name)), overload_set_(std::move(overload_set)) {}

    const std::string& get() const { return name_; }

    const SymbolRef* get_symbol_ref() const override { return this; }

    std::unique_ptr<Value> copy() const override;

    // return our overload set (possible symbols we may adopt)
    const std::deque<std::reference_wrapper<symbol::Symbol>>& candidates() const { return overload_set_; }

    // attempt to resolve this symbol, populates lvalue with ::Symbol
    bool resolve(const message::MessageGenerator &source, optional_ref<message::List> messages, optional_ref<const ast::type::Node> type_hint = {});

    bool materialise(Context &ctx, const MaterialisationOptions &options) override;
  };

  // create a Value which is a symbol reference, populate overload set from ctx
  std::unique_ptr<SymbolRef> symbol_ref(const std::string name, const symbol::SymbolTable& symbols);

  // we will resolve into a literal of the given type
  // this is for word-sized literals only (non-reference types)
  class Literal : public Value {
    const memory::Literal& lit_;

  public:
    explicit Literal(const memory::Literal& lit) : lit_(lit), Value(lit.type()) {}

    std::unique_ptr<Value> copy() const override;

    const memory::Literal& get() const { return lit_; }

    const Literal* get_literal() const override { return this; }

    bool materialise(Context &ctx, const MaterialisationOptions &options = {}) override;
  };

  // create a literal
  std::unique_ptr<Value> literal(const memory::Literal& lit);

  // create a literal rvalue
  std::unique_ptr<Value> literal(const memory::Literal& lit, const memory::Ref& ref);

  // we represent a sequence of literals which will be placed in contiguous memory
  // this is for word-sized literals only (non-reference types)
  class ContiguousLiteral : public Value {
  public:
    using Elements = std::deque<std::reference_wrapper<value::Value>>;

  private:
    Elements elements_;
    bool global_; // define globally or locally?

  public:
    ContiguousLiteral(const ast::type::Node& type, Elements elements, bool is_global);

    std::unique_ptr<Value> copy() const override;

    bool materialise(Context &ctx, const MaterialisationOptions &options = {}) override;
  };

  // create a literal
  std::unique_ptr<Value> contiguous_literal(const ast::type::Node& type, ContiguousLiteral::Elements elements, bool is_global);
}
