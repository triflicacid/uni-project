#pragma once
#include "function.hpp"
#include "operators/info.hpp"

namespace lang::ast {
  class BlockNode;

  class OperatorDefinitionNode : public FunctionNode {
    bool _process(lang::Context &ctx) override;

    std::string block_prefix() const override { return "operator" + name().image; }

  public:
    using FunctionNode::FunctionNode;

    std::string node_name() const override { return "operator overload"; }

    // get operator info
    const ops::OperatorInfo& info() const;
  };
}