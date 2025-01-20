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

    std::string name() const override { return "type::function"; }

    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    size_t size() const override { return 8; } // size of an address
  };
}
