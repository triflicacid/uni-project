#pragma once
#include "function.hpp"
#include "operators/info.hpp"

namespace lang::ast {
  class BlockNode;

  /** @brief Represents a user-defined `operator<sym>(...)` overload definition, a `FunctionNode` that also registers itself as an `ops::UserDefinedOperator`. */
  class OperatorDefinitionNode : public FunctionNode {
    /** @brief Return the prefix "operator<sym>" preceding the parameter list. */
    std::string block_prefix() const override { return "operator" + name().image; }

  public:
    using FunctionNode::FunctionNode;

    /** @brief Return the node kind name, "operator overload". */
    std::string node_name() const override { return "operator overload"; }

    /** @brief Return the precedence/associativity/overloadability info for this operator's symbol, given the number of parameters. */
    const ops::OperatorInfo& info() const;

    /**
     * @brief Register this function's symbol, then register it as a user-defined operator overload, rejecting non-overloadable or duplicate signatures.
     * @param messages Message list to report errors into.
     * @param registry Registry to create the function's symbol in.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;
  };
}