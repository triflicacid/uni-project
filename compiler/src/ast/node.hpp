#pragma once

#include "printable_entity.hpp"
#include <deque>
#include <variant>
#include "messages/list.hpp"
#include "lexer/token.hpp"
#include "value/value.hpp"
#include "optional_ref.hpp"
#include "control_flow/conditional_context.hpp"
#include "memory/storage_location.hpp"

namespace lang {
  struct Context;

  namespace symbol {
    class Registry;
  }
}

/** @brief Abstract syntax tree: the parsed representation of Edel source, processed through the process/resolve/generate_code compilation phases. */
namespace lang::ast {
  /**
   * @brief Abstract base of every AST node, the type-checking/codegen counterpart of type::Node's type-system hierarchy.
   *
   * Carries the node's source token span, an optional type hint used for overload
   * resolution, an optional conditional context set while evaluating a guard, an
   * optional storage-location target for where the result should be placed, and a
   * `value_` populated during `process`. Declares the four-phase compilation
   * pipeline: `collate_registry` (phase 1, symbol collection), pure-virtual
   * `process` (phase 2, validation), `resolve` (phase 3, ambiguity resolution),
   * and `generate_code` (phase 4, code emission).
   */
  class Node : public PrintableEntity, public lexer::TokenSpan {
    lexer::Token tstart_;
    std::optional<lexer::Token> tend_;
    optional_ref<const type::Node> type_hint_; // type hint, used for resolving overload sets etc
    optional_ref<control_flow::ConditionalContext> cond_ctx_; // set when evaluating a conditional, means operator should support this and contribute
    optional_ref<const memory::StorageLocation> target_; // store result at this target?

  protected:
    std::unique_ptr<value::Value> value_; // every node has a value, which is possibly set in ::process

  public:
    /**
     * @brief Construct a node starting at the given token.
     * @param token First token of the node's source span.
     */
    explicit Node(lexer::Token token) : PrintableEntity(), tstart_(token) {}

    /** @brief Return the first token of this node's source span. */
    const lexer::Token& token_start() const override final { return tstart_; }

    /** @brief Return the last token of this node's source span, or the start token if none was set. */
    const lexer::Token& token_end() const override final { return tend_ ? *tend_ : tstart_; }

    /**
     * @brief Set the first token of this node's source span.
     * @param token New start token.
     */
    void token_start(const lexer::Token& token) { tstart_ = token; }

    /**
     * @brief Set the last token of this node's source span.
     * @param token New end token.
     */
    void token_end(const lexer::Token& token) { tend_ = token; }

    /** @brief Return the type hint attached to this node, if any. */
    const optional_ref<const type::Node>& type_hint() const { return type_hint_; }

    /**
     * @brief Set this node's type hint.
     * @param hint Type to use as a hint, e.g. when resolving overload sets.
     */
    void type_hint(const type::Node& hint) { type_hint_ = hint; }

    /**
     * @brief Set or clear this node's type hint.
     * @param hint Type hint to set, or an empty optional to clear it.
     */
    void type_hint(optional_ref<const type::Node> hint) { type_hint_ = std::move(hint); }

    /** @brief Return the conditional context this node is being evaluated under, if any. */
    const optional_ref<control_flow::ConditionalContext>& conditional_context() const { return cond_ctx_; }

    /**
     * @brief Mark this node as being evaluated within a conditional context, so operators can contribute branch logic directly.
     * @param ctx Conditional context to attach.
     */
    void conditional_context(control_flow::ConditionalContext& ctx) { cond_ctx_ = std::ref(ctx); }

    /** @brief Return the storage location this node's result should be placed at, if any. */
    const optional_ref<const memory::StorageLocation>& target() const { return target_; }

    /**
     * @brief Set the storage location this node's result should be placed at.
     * @param t Target storage location.
     */
    void target(const memory::StorageLocation& t) { target_ = std::cref(t); }

    /** @brief Test whether this node unconditionally returns from the enclosing function, used to check that control reaches the end of a function correctly. */
    virtual bool always_returns() const { return false; }

    /** @brief Test whether this node may leave its result in `$ret` rather than requiring it to be moved elsewhere. */
    virtual bool writes_to_ret() const { return false; }

    /** @brief Return the value representing the result of this node. */
    virtual value::Value& value() const;

    /**
     * @brief Print this node (and by default only its name) in tree form for debugging/`--ast` output.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    virtual std::ostream& print_tree(std::ostream& os, unsigned int indent_level = 0) const;

    /**
     * @brief Phase 1: collect symbols declared by this node's children into the registry, ahead of processing.
     *
     * `let ...` declarations must not be added here, since variables cannot be
     * referenced before their own assignment.
     * @param messages Message list to report errors into.
     * @param registry Registry to collect symbols into.
     * @return True on success.
     */
    virtual bool collate_registry(message::List& messages, symbol::Registry& registry);

    /**
     * @brief Phase 2: validate and process this node, reporting any errors, without generating code.
     *
     * May only call `process`/`resolve` on other nodes; no other phase may run yet.
     * @param ctx Compilation context.
     * @return True on success.
     */
    virtual bool process(Context& ctx) = 0;

    /**
     * @brief Phase 3: resolve ambiguities left after processing, e.g. disambiguating symbol references. Never generates code.
     * @param ctx Compilation context.
     * @return True on success.
     */
    virtual bool resolve(Context& ctx) { return true; }

    /**
     * @brief Phase 4: generate assembly code for this node, assuming all prior phases succeeded without error.
     * @param ctx Compilation context.
     * @return True on success.
     */
    virtual bool generate_code(Context& ctx);
  };

  /**
   * @brief Write indentation to an output stream.
   * @param os Output stream to write to.
   * @param level Indentation depth, in indentation units.
   * @return The same stream, for chaining.
   */
  std::ostream& indent(std::ostream& os, unsigned int level);

  /**
   * @brief Process and resolve a node, then check its resulting value is of the expected l/rvalue category.
   * @param node Node to process and resolve.
   * @param ctx Compilation context.
   * @param expect_lvalue Whether the node's value is expected to be an lvalue (an rvalue is always required).
   * @return The node's value if processing succeeded and it matches the expected category, else nothing.
   */
  optional_ref<const value::Value> process_node_and_expect(Node& node, Context& ctx, bool expect_lvalue);

  /** @brief Mixin for a node that holds an ordered sequence of child nodes appended incrementally by the parser. */
  struct ContainerNode {
    /**
     * @brief Append a single child node.
     * @param ast_node Node to append.
     */
    virtual void add(std::unique_ptr<Node> ast_node) = 0;

    /**
     * @brief Append a sequence of child nodes.
     * @param ast_nodes Nodes to append.
     */
    virtual void add(std::deque<std::unique_ptr<Node>> ast_nodes) = 0;
  };
}
