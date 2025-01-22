#pragma once
#include "messages/list.hpp"
#include "lexer/token.hpp"

namespace lang {
  struct Context;
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

    // validate/process this Node, populating the message queue if necessary
    // return if successful
    virtual bool process(Context& ctx);
  };

  // utility: write a certain indent to the output stream
  std::ostream& indent(std::ostream& os, unsigned int level);
}
