#pragma once

#include <string>
#include <optional>
#include <memory>
#include <deque>

namespace lang::ast::type {
  class FunctionNode;
}

namespace lang::ops {
  using OperatorId = unsigned int;

  class Operator {
    OperatorId id_;
    std::string symbol_;
    const ast::type::FunctionNode& type_;

  public:
    Operator(std::string symbol, const ast::type::FunctionNode& type);

    virtual ~Operator() = default;

    OperatorId id() const { return id_; }

    const std::string& symbol() const { return symbol_; }

    const ast::type::FunctionNode& type() const { return type_; }

    std::ostream& print_code(std::ostream& os) const;

    // are we built-in or overloaded
    virtual bool builtin() const = 0;
  };

  // get a list of references of operators with this name
  std::deque<std::reference_wrapper<const Operator>> get(const std::string symbol);

  // get a reference to the given operator, return None if it does not exist
  std::optional<std::reference_wrapper<const Operator>> get(const std::string symbol, const ast::type::FunctionNode& type);

  // add operator to store
  void store_operator(std::unique_ptr<Operator> op);
}
