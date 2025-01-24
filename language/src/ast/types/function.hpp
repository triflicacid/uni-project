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
    FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters, std::optional<std::reference_wrapper<const Node>> returns) : parameters_(std::move(parameters)), returns_(returns) {}

    std::string name() const override { return "function"; }

    // get the type of the ith argument
    const Node& arg(int i) const { return parameters_[i]; }

    // get the return
    std::optional<std::reference_wrapper<const Node>> returns() const { return returns_; }

    const FunctionNode* get_func() const override { return this; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    size_t size() const override { return 8; } // size of an address

    std::string to_label() const override;
  };
}
