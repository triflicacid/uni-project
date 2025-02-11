#pragma once

#include <deque>
#include "node.hpp"
#include "unit.hpp"
#include "optional_ref.hpp"

namespace lang::ast::type {
  class FunctionNode : public Node {
    std::deque<std::reference_wrapper<const Node>> parameters_;
    const Node& returns_;

  public:
    explicit FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters) : parameters_(std::move(parameters)), returns_(unit) {}
    FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters, const Node& returns) : parameters_(std::move(parameters)), returns_(returns) {}

    std::string node_name() const override { return "function"; }

    // get the number of arguments
    size_t args() const { return parameters_.size(); }

    // get the type of the ith argument
    const Node& arg(int i) const { return parameters_[i]; }

    // get the return value
    const Node& returns() const { return returns_; }

    const FunctionNode* get_func() const override { return this; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    std::ostream& print_code(std::ostream &os, bool print_return, unsigned int indent_level = 0) const;

    size_t size() const override { return 8; } // size of an address

    constants::inst::datatype::dt get_asm_datatype() const override;

    std::string to_label() const override;

    // 'create' a new function type -- if exists, return reference, otherwise create new type and return
    // if provided, the return type is also used in comparison
    // if there is no match, a new function type is created with the given return type (or unit if not provided)
    static const FunctionNode& create(const std::deque<std::reference_wrapper<const Node>>& parameters, optional_ref<const Node> returns);

    // same as static ::filter_candidates, but uses this type's parameter list
    std::deque<std::reference_wrapper<const FunctionNode>> filter_candidates(const std::deque<std::reference_wrapper<const FunctionNode>>& options) const;

    // given list of parameter types, return list of possible candidates from input list of options
    // if there is a perfect match, return this
    // otherwise, there are selection issues, so return all suitable candidates
    static std::deque<std::reference_wrapper<const FunctionNode>> filter_candidates(const std::deque<std::reference_wrapper<const Node>>& parameters, const std::deque<std::reference_wrapper<const FunctionNode>>& options);
  };
}
