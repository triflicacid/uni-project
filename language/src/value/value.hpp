#pragma once

#include <optional>
#include <cassert>
#include <functional>
#include "memory/ref.hpp"
#include "memory/storage_location.hpp"

namespace lang::ast::type {
  class Node;
}

namespace lang::value {
  class Symbol;
  class SymbolRef;
  class Literal;
  class Temporary;

  // options used when creating a Value
  struct Options {
    std::optional<memory::Ref> rvalue; // reference for an rvalue
    std::optional<std::reference_wrapper<const memory::StorageLocation>> lvalue; // location for an lvalue
    bool computable = false; // even if we are not an rvalue currently, can we be computed to become one in the future
  };

  class Value {
    std::optional<memory::Ref> ref_; // where this item is stored/was placed by the RegisterAllocationManager, opptional as Value can be created prior to storage
    std::optional<std::reference_wrapper<const memory::StorageLocation>> source_;
    bool computable_ = false;

  public:
    Value(const Options& options) : ref_(options.rvalue), source_(options.lvalue), computable_(options.computable) {}

    virtual ~Value() = default;

    // are we an lvalue, i.e., are we stored somewhere
    bool is_lvalue() const { return source_.has_value(); }

    // are we an rvalue, i.e., are we in a register (have a computed value)
    bool is_rvalue() const { return ref_.has_value(); }

    // are we computable - used for "to-be" rvalues
    bool is_computable() const { return computable_ && !ref_; }

    // get reference, error if not rvalue
    const memory::Ref& ref() const { assert(is_rvalue()); return ref_.value(); }

    // get storage location, error if not lvalue
    const memory::StorageLocation& source() const { assert(is_lvalue()); return source_.value(); }

    // set reference when loaded
    void set_ref(const memory::Ref& ref) { ref_ = ref; }

    virtual const ast::type::Node& type() const = 0;

    virtual const Symbol* get_symbol() const { return nullptr; }

    virtual const SymbolRef* get_symbol_ref() const { return nullptr; }

    virtual const Literal* get_literal() const { return nullptr; }

    virtual const Temporary* get_temporary() const { return nullptr; }
  };
}
