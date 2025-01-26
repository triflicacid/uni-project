#pragma once

#include <deque>
#include "node.hpp"

namespace lang::ast::type {
  class FunctionNode : public Node {
    std::deque<std::reference_wrapper<const Node>> parameters_;
    std::optional<std::reference_wrapper<const Node>> returns_;

  public:
    explicit FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters) : parameters_(std::move(parameters)) {}
    FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters, const Node& returns) : parameters_(std::move(parameters)), returns_(returns) {}
    FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters, std::optional<std::reference_wrapper<const Node>> returns) : parameters_(std::move(parameters)), returns_(std::move(returns)) {}

    std::string name() const override { return "function"; }

    // get the number of arguments
    size_t args() const { return parameters_.size(); }

    // get the type of the ith argument
    const Node& arg(int i) const { return parameters_[i]; }

    // get the return
    std::optional<std::reference_wrapper<const Node>> returns() const { return returns_; }

    // set the return value
    void returns(const Node& type) { returns_ = type; }

    const FunctionNode* get_func() const override { return this; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    size_t size() const override { return 8; } // size of an address

    // returns if we are a subtype of the other function, i.e., `this :> other`
    // same as `TypeGraph::is_subtype(this, other)`
    // functions are subtypes iff all arguments are subtypes or equal
    bool is_subtype(const FunctionNode& parent) const;

    std::string to_label() const override;

    // 'create' a new function type -- if exists, return reference, otherwise create new type and return
    // the return type is added to new creations only and is not used in comparisons
    static const FunctionNode& create(const std::deque<std::reference_wrapper<const Node>>& parameters, const std::optional<std::reference_wrapper<const Node>>& returns);
  };
}
