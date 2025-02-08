#pragma once

#include <deque>
#include "messages/list.hpp"
#include "lexer/token.hpp"
#include "value/value.hpp"

namespace lang {
  struct Context;

  namespace symbol {
    class Registry;
  }
}

namespace lang::ast {
  class NodeBase {
  public:
    virtual ~NodeBase() = default;

    virtual std::string node_name() const = 0;

    // print in code form
    virtual std::ostream& print_code(std::ostream& os, unsigned int indent_level = 0) const = 0;
  };

class Node : public NodeBase, public lexer::TokenSpan {
    lexer::Token tstart_;
    std::optional<lexer::Token> tend_;

  protected:
    std::unique_ptr<value::Value> value_; // every node has a value, which is possibly set in ::process

  public:
    explicit Node(lexer::Token token) : NodeBase(), tstart_(token) {}

    const lexer::Token& token_start() const override final { return tstart_; }

    const lexer::Token& token_end() const override final { return tend_ ? *tend_ : tstart_; }

  void token_start(const lexer::Token& token) { tstart_ = token; }

    void token_end(const lexer::Token& token) { tend_ = token; }

    // refer to the value represents the result of this node
    virtual const value::Value& value() const;

    // populate/resolve the lvalue component of our value, if possible
    // returns `false` if error occurred
    virtual bool resolve_lvalue(Context& ctx) { return true; }

    // populate/resolve the rvalue component of our value, if possible
    // returns `false` if error occurred
    virtual bool resolve_rvalue(Context& ctx) { return true; }

    // fully resolve our value, if possible (both l and rvalue)
    // returns `false` if error occurred
    bool resolve_value(Context& ctx);

    // print in tree form, default only print name
    virtual std::ostream& print_tree(std::ostream& os, unsigned int indent_level = 0) const;

    // Phase 1:
    // collect symbols from children of this node
    // these will be inserted into the SymbolTable prior to processing
    // note that `let ...` should *not* be added here, as variables cannot be used prior to assignment
    // return if success
    virtual bool collate_registry(message::List& messages, symbol::Registry& registry) { return true; }

    // Phase 2:
    // validate/process this Node, populating the message queue if necessary
    // return if successful
    // MUST be overridden
    virtual bool process(Context& ctx);
  };

  // utility: write a certain indent to the output stream
  std::ostream& indent(std::ostream& os, unsigned int level);

  // utility: node which contains other nodes
  struct ContainerNode {
    virtual void add(std::unique_ptr<Node> ast_node) = 0;

    virtual void add(std::deque<std::unique_ptr<Node>> ast_nodes) = 0;
  };
}
