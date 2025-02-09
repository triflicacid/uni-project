#pragma once

#include <deque>
#include "node.hpp"
#include "unit.hpp"
#include "optional_ref.hpp"

namespace lang::ast::type {
  class FunctionNode : public Node {
    std::deque<std::reference_wrapper<const Node>> parameters_;
    std::reference_wrapper<const Node> returns_;

  public:
    explicit FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters) : parameters_(std::move(parameters)), returns_(unit) {}
    FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters, const Node& returns) : parameters_(std::move(parameters)), returns_(returns) {}
    FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters, std::optional<std::reference_wrapper<const Node>> returns)
      : parameters_(std::move(parameters)), returns_(returns.has_value() ? returns.value() : unit) {}

    std::string node_name() const override { return "function"; }

    // get the number of arguments
    size_t args() const { return parameters_.size(); }

    // get the type of the ith argument
    const Node& arg(int i) const { return parameters_[i]; }

    // get the return value
    const Node& returns() const { return returns_; }

    // set the return value, this can be done as only the param types are used for candidate selection
    void returns(const Node& type) { returns_ = type; }

    const FunctionNode* get_func() const override { return this; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    size_t size() const override { return 8; } // size of an address

    constants::inst::datatype::dt get_asm_datatype() const override;

    // returns if we are a subtype of the other function, i.e., `this :> other`
    // same as `TypeGraph::is_subtype(this, other)`
    // functions are subtypes iff all arguments are subtypes or equal
    bool is_subtype(const FunctionNode& parent) const;

    std::string to_label() const override;

    // 'create' a new function type -- if exists, return reference, otherwise create new type and return
    // the return type is added to new creations only and is not used in comparisons
    // if return type is std::nullopt, only params are matched, otherwise return type is too (must match)
    static const FunctionNode& create(const std::deque<std::reference_wrapper<const Node>>& parameters, const std::optional<std::reference_wrapper<const Node>>& returns);

    // same as static ::filter_candidates, but uses this type's parameter list
    std::deque<std::reference_wrapper<const FunctionNode>> filter_candidates(const std::deque<std::reference_wrapper<const FunctionNode>>& options) const;

    // same as static ::select_candidates, but uses this type's parameter list
    optional_ref<const FunctionNode> select_candidate(const std::deque<std::reference_wrapper<const FunctionNode>>& options, message::List& messages) const;

    // given list of parameter types, return list of possible candidates from input list of options
    // if there is a perfect match, return this
    // otherwise, there are selection issues, so return all suitable candidates
    static std::deque<std::reference_wrapper<const FunctionNode>> filter_candidates(const std::deque<std::reference_wrapper<const Node>>& parameters, const std::deque<std::reference_wrapper<const FunctionNode>>& options);

    // runs ::filter_candidates on first two parameters
    // if |candidates|=1, return it
    // otherwise, generate error messages
    static optional_ref<const FunctionNode> select_candidate(const FunctionNode& target, std::deque<std::reference_wrapper<const FunctionNode>> options, message::List& messages);
  };
}
