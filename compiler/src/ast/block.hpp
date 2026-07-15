#pragma once

#include <deque>
#include "node.hpp"
#include "symbol/registry.hpp"

namespace lang::ast {
  /**
   * @brief Represents a `{ ... }` code block, an ordered sequence of statement nodes.
   *
   * Optionally introduces a new lexical scope backed by its own `symbol::Registry`,
   * and can be marked (via `make_expr`) to act as an expression whose value is
   * that of its last line.
   */
  class BlockNode : public Node, public ContainerNode {
    std::deque<std::unique_ptr<Node>> lines_;
    bool scope_ = true; // add a new scope
    std::unique_ptr<symbol::Registry> registry_; // local registry, NULL if !scope_
    bool returns_ = false; // does this block return a value?

  public:
    using Node::Node;

    /** @brief Return the node kind name, "block". */
    std::string node_name() const override { return "block"; }

    /**
     * @brief Append a single statement to this block.
     * @param ast_node Statement to append.
     */
    void add(std::unique_ptr<Node> ast_node) override;

    /**
     * @brief Append a sequence of statements to this block.
     * @param ast_nodes Statements to append.
     */
    void add(std::deque<std::unique_ptr<Node>> ast_nodes) override;

    /**
     * @brief Set whether this block introduces a new lexical scope.
     * @param b True to introduce a new scope.
     */
    void add_new_scope(bool b) { scope_ = b; }

    /** @brief Mark this block as returning a value, i.e. behaving as an expression whose value is its last line's. */
    void make_expr() { returns_ = true; }

    /** @brief Test whether any statement in this block unconditionally returns from the enclosing function. */
    bool always_returns() const override;

    /** @brief Test whether any statement in this block may write to `$ret`. */
    bool writes_to_ret() const override;

    /** @brief Return this block's value: its last line's value if marked as an expression, else the default unit value. */
    value::Value& value() const override;

    /**
     * @brief Print this block as `{ ... }` source code, with each line indented.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this block and its lines in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Collect symbols from this block's lines into a fresh local registry if this block introduces a scope, else the given one.
     * @param messages Message list to report errors into.
     * @param registry Enclosing registry to collate into if this block has no scope of its own.
     * @return True on success.
     */
    bool collate_registry(message::List& messages, symbol::Registry &registry) override;

    /**
     * @brief Push a new symbol-table scope (if applicable), process each line, warn on empty blocks and unreachable code, then pop the scope.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Resolve ambiguities in each line, stopping at unreachable code.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool resolve(lang::Context &ctx) override;

    /**
     * @brief Generate code for each line, forwarding this block's target to its last line, stopping at unreachable code.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };
}