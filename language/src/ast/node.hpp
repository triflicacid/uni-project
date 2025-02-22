#pragma once

#include <deque>
#include "messages/list.hpp"
#include "lexer/token.hpp"
#include "value/value.hpp"
#include "optional_ref.hpp"
#include "control-flow/conditional_context.hpp"
#include "control-flow/loop_context.hpp"

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
    optional_ref<const ast::type::Node> type_hint_; // type hint, used for resolving overload sets etc
    optional_ref<control_flow::ConditionalContext> cond_ctx_; // set when evaluating a conditional, means operator should support this and contribute
    optional_ref<control_flow::LoopContext> loop_ctx_; // set when inside a loop, used for `break` and `continue`

  protected:
    std::unique_ptr<value::Value> value_; // every node has a value, which is possibly set in ::process

  public:
    explicit Node(lexer::Token token) : NodeBase(), tstart_(token) {}

    const lexer::Token& token_start() const override final { return tstart_; }

    const lexer::Token& token_end() const override final { return tend_ ? *tend_ : tstart_; }

    void token_start(const lexer::Token& token) { tstart_ = token; }

    void token_end(const lexer::Token& token) { tend_ = token; }

    const optional_ref<const ast::type::Node>& type_hint() const { return type_hint_; }

    // set our type hint
    void type_hint(const ast::type::Node& hint) { type_hint_ = hint; }
    void type_hint(optional_ref<const ast::type::Node> hint) { type_hint_ = std::move(hint); }

    const optional_ref<control_flow::ConditionalContext>& conditional_context() const { return cond_ctx_; }
    void conditional_context(control_flow::ConditionalContext& ctx) { cond_ctx_ = std::ref(ctx); }

    const optional_ref<control_flow::LoopContext>& loop_context() const { return loop_ctx_; }
    void loop_context(control_flow::LoopContext& ctx) { loop_ctx_ = std::ref(ctx); }

    // does this node return absolutely from a function?
    // used to check if control reaches the end of a function
    virtual bool always_returns() const { return false; }

    // refer to the value represents the result of this node
    virtual const value::Value& value() const;

    // print in tree form, default only print name
    virtual std::ostream& print_tree(std::ostream& os, unsigned int indent_level = 0) const;

    // Phase 1:
    // collect symbols from children of this node
    // these will be inserted into the SymbolTable prior to processing
    // note that `let ...` should *not* be added here, as variables cannot be used prior to assignment
    // return if success
    virtual bool collate_registry(message::List& messages, symbol::Registry& registry);

    // Phase 2:
    // validate/process this Node, populating the message queue if necessary
    // does not generate any code
    // may ONLY call ::process and ::resolve on another node (no other phases permitted)
    // must be overridden
    // return if success
    virtual bool process(Context& ctx) = 0;

    // Phase 3:
    // function used to resolve ambiguities, specifically in symbol references
    // guaranteed not to generate any code
    // return if success
    virtual bool resolve(Context& ctx) { return true; }

    // Phase 4:
    // actually generates assembly code
    // by this point, there should be no errors, and node should have all the information it needs
    // return if success
    virtual bool generate_code(Context& ctx) const;
  };

  // utility: write a certain indent to the output stream
  std::ostream& indent(std::ostream& os, unsigned int level);

  // process & resolve given node, expect l-- or r-value, return Value produced (or nothing)
  optional_ref<const value::Value> process_node_and_expect(Node& node, Context& ctx, bool expect_lvalue);

  // utility: node which contains other nodes
  struct ContainerNode {
    virtual void add(std::unique_ptr<Node> ast_node) = 0;

    virtual void add(std::deque<std::unique_ptr<Node>> ast_nodes) = 0;
  };
}
