#pragma once
#include "messages/list.hpp"
#include "lexer/token.hpp"

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

    virtual std::string name() const = 0;

    // print in code form
    virtual std::ostream& print_code(std::ostream& os, unsigned int indent_level = 0) const = 0;
  };

  class Node : public NodeBase {
  protected:
    lexer::Token token_; // token we began with

  public:
    explicit Node(lexer::Token token) : NodeBase(), token_(token) {}

    const lexer::Token& token() const { return token_; }

    // print in tree form, default only print name
    virtual std::ostream& print_tree(std::ostream& os, unsigned int indent_level = 0) const;

    // collect symbols from children of this node
    // these will be inserted into the SymbolTable prior to processing
    // return if success
    virtual bool collate_registry(message::List& messages, symbol::Registry& registry) { return true; }

    // validate/process this Node, populating the message queue if necessary
    // return if successful
    // MUST be overridden
    virtual bool process(Context& ctx);
  };

  // utility: write a certain indent to the output stream
  std::ostream& indent(std::ostream& os, unsigned int level);
}
