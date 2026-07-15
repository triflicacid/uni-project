#pragma once

#include "node.hpp"
#include "block.hpp"

namespace lang::ast {
  /**
   * @brief Represents a `namespace a.b.c { ... }` declaration.
   *
   * Distinct from type::NamespaceNode, the type-system marker type. Stores the
   * dotted path as a sequence of name tokens, the body's child lines, a local
   * registry for the body, and the symbol id of the topmost path segment.
   */
  class NamespaceNode : public Node, public ContainerNode {
    std::deque<lexer::Token> names_; // <name1>.<name2> ...
    std::deque<std::unique_ptr<Node>> lines_;
    std::unique_ptr<symbol::Registry> registry_; // local registry
    std::optional<symbol::SymbolId> id_; // IF of *topmost* namespace

  public:
    /**
     * @brief Construct an empty namespace declaration.
     * @param token Start token of the declaration.
     * @param names Dotted path of name tokens (e.g. `a`, `b`, `c`).
     */
    NamespaceNode(lexer::Token token, std::deque<lexer::Token> names) : Node(std::move(token)), names_(std::move(names)) {}

    /**
     * @brief Construct a namespace declaration with a pre-populated body.
     * @param token Start token of the declaration.
     * @param names Dotted path of name tokens.
     * @param lines Body statements.
     */
    NamespaceNode(lexer::Token token, std::deque<lexer::Token> names, std::deque<std::unique_ptr<Node>> lines)
        : Node(std::move(token)), names_(std::move(names)), lines_(std::move(lines)) {}

    /** @brief Return the node kind name, "namespace". */
    std::string node_name() const override { return "namespace"; }

    /** @brief Return the dotted path as a single string, e.g. "a.b.c". */
    std::string name() const;

    /**
     * @brief Append a single statement to this namespace's body.
     * @param ast_node Statement to append.
     */
    void add(std::unique_ptr<Node> ast_node) override;

    /**
     * @brief Append a sequence of statements to this namespace's body.
     * @param ast_nodes Statements to append.
     */
    void add(std::deque<std::unique_ptr<Node>> ast_nodes) override;

    /**
     * @brief Print this namespace as `namespace <path> { ... }` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this namespace and its body in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Create (or resolve) a namespace symbol for each path segment, verifying earlier declarations of the same path are also namespaces, then collate the body's symbols into a local registry.
     * @param messages Message list to report errors into.
     * @param registry Enclosing registry to create the path's namespace symbols in.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Push the namespace path onto the symbol table, insert the local registry, process and resolve each body line, then pop the path.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code for each body line.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };
}
