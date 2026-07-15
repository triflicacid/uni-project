#pragma once

#include "ast/node.hpp"
#include "operators/info.hpp"
#include "optional_ref.hpp"
#include "memory/storage_location.hpp"
#include <unordered_set>

namespace lang {
  namespace ops {
    class Operator;
  }

  namespace symbol {
    class Symbol;
  }

  namespace type {
    class FunctionNode;
  }
}

namespace lang::ast {
  /** @brief Base class for all built-in/overloadable operator expression nodes, storing the operator's symbol token and its operand children. */
  class OperatorNode : public Node {
  protected:
    lexer::Token op_symbol_; // token of our actual symbol
    std::deque<std::unique_ptr<Node>> args_;

    /**
     * @brief Return the ith operand.
     * @param i Index of the operand.
     * @return Reference to the operand node.
     */
    Node& arg(int i) const;

    /**
     * @brief Check that the ith operand's value is an lvalue or rvalue as expected, reporting an error otherwise.
     * @param i Index of the operand to check.
     * @param messages Message list to report an error into on mismatch.
     * @param expect_lvalue Whether the operand is expected to be an lvalue (false expects an rvalue).
     * @return True if the operand matches the expected category.
     */
    bool expect_arg_lrvalue(int i, message::List& messages, bool expect_lvalue) const;

  public:
    /**
     * @brief Construct an operator node over a set of operands.
     * @param token Start token of the expression.
     * @param symbol Token of the operator's symbol.
     * @param args Operand nodes.
     */
    OperatorNode(lexer::Token token, lexer::Token symbol, std::deque<std::unique_ptr<Node>> args)
      : Node(std::move(token)), op_symbol_(std::move(symbol)), args_(std::move(args)) {
      if (!args_.empty()) token_end(args_.back()->token_end());
    }

    /** @brief Return the operator's symbol text (e.g. "+", "&&"). */
    virtual const std::string& symbol() const { return op_symbol_.image; }

    /** @brief Return the node kind name, "unary operator" or "binary operator" depending on arity. */
    std::string node_name() const override;

    /** @brief Test whether any operand may write to `$ret`. */
    bool writes_to_ret() const override;

    /**
     * @brief Print this operator expression as `(lhs op rhs)` or `(op expr)` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this operator expression and its operands in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Collect symbols from each operand.
     * @param messages Message list to report errors into.
     * @param registry Registry to collect symbols into.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Process each operand.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Create the appropriate operator node for a unary expression, special-casing `sizeof`, `&`, and `*`.
     * @param token Operator token.
     * @param expr Operand expression.
     * @return Newly constructed operator node.
     */
    static std::unique_ptr<OperatorNode> unary(lexer::Token token, std::unique_ptr<Node> expr);

    /**
     * @brief Create the appropriate operator node for a binary expression, special-casing `.`, `=`, `[]`, `&&`, and `||`.
     * @param token Start token of the expression.
     * @param symbol Operator symbol token.
     * @param lhs Left-hand operand.
     * @param rhs Right-hand operand.
     * @return Newly constructed operator node.
     */
    static std::unique_ptr<OperatorNode> binary(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);
  };

  /** @brief Represents an operator resolved via user/builtin overload lookup (`operatorX(...)`), caching the matched signature and operator. */
  class OverloadableOperatorNode : public OperatorNode {
    optional_ref<const type::FunctionNode> signature_; // signature, set in ::process
    optional_ref<const ops::Operator> op_; // resolves operator, set in ::process
    bool special_pointer_op_ = false; // track if +/- on a pointer as we need to do something special

  public:
    using OperatorNode::OperatorNode;

    /** @brief Render this operator invocation as "operatorX(...)" text, using its resolved signature if known. */
    std::string to_string() const;

    /** @brief Test whether the resolved operator is user-defined (and so may write to `$ret`), or any operand may. */
    bool writes_to_ret() const override;

    /**
     * @brief Process operands (special-casing pointer +/- arithmetic) and resolve the matching operator overload.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code invoking the resolved operator, or the pointer-arithmetic special case.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };

  /** @brief Represents `expr as type`, an optionally unchecked cast to a target type. */
  class CastOperatorNode : public OperatorNode {
    const type::Node& target_;
    bool sudo_ = false;

  public:
    /**
     * @brief Construct a cast expression.
     * @param token Start token of the expression.
     * @param symbol Token of the `as` keyword.
     * @param target Target type to cast to.
     * @param expr Expression being cast.
     * @param sudo Whether to bypass the usual cast compatibility checks.
     */
    CastOperatorNode(lexer::Token token, lexer::Token symbol, const type::Node& target, std::unique_ptr<Node> expr, bool sudo = false);

    /**
     * @brief Print this cast as `expr as type` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this cast and its operand in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Process the operand with the target type as a hint, rejecting zero-sized targets.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate the operand's code then emit a datatype conversion to the target type.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };

  /** @brief Represents `lhs.rhs` member/property access, resolving the property name against the LHS's type. */
  class DotOperatorNode : public OperatorNode {
    std::string property_;

  public:
    /**
     * @brief Construct a member-access expression.
     * @param token Start token of the expression.
     * @param symbol Token of the `.` operator.
     * @param lhs Subject expression.
     * @param rhs Property-name expression (must resolve to a bare symbol reference).
     */
    DotOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    /**
     * @brief Process the LHS and resolve the RHS as a property name, either combining it into a namespace path or looking it up on the LHS's type.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Resolve a pending symbol reference produced when the LHS is a namespace.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool resolve(Context& ctx) override;

    /**
     * @brief Generate code loading the resolved symbol, or fetching the property's value from the LHS.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };

  /** @brief Represents `lhs = rhs`, assigning the RHS's value into the LHS's lvalue location. */
  class AssignmentOperatorNode : public OperatorNode {
  public:
    /**
     * @brief Construct an assignment expression.
     * @param token Start token of the expression.
     * @param symbol Token of the `=` operator.
     * @param lhs Assignment target.
     * @param rhs Value being assigned.
     */
    AssignmentOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    /**
     * @brief Process both operands, rejecting assignment to constants and mismatched types.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code evaluating the RHS and storing it into the LHS's location.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };

  /** @brief Represents `&expr`, computing the address of an lvalue operand. */
  class AddressOfOperatorNode : public OperatorNode {
  public:
    /**
     * @brief Construct an address-of expression.
     * @param token Start token of the expression.
     * @param symbol Token of the `&` operator.
     * @param expr Operand whose address is taken.
     */
    AddressOfOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> expr);

    /**
     * @brief Process the operand, requiring it to be an lvalue or reference-like value.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code producing the operand's address.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };

  /** @brief Represents `*expr`, reading the value stored at the address held by the operand. */
  class DereferenceOperatorNode : public OperatorNode {
  public:
    /**
     * @brief Construct a dereference expression.
     * @param token Start token of the expression.
     * @param symbol Token of the `*` operator.
     * @param expr Pointer/array-typed operand being dereferenced.
     */
    DereferenceOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> expr);

    /**
     * @brief Process the operand, requiring it to have a pointer or array type.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code loading the pointed-to value.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };

  /** @brief Represents a generalized function call `expr(<args>)`, where the callee is an arbitrary expression rather than a fixed symbol. */
  class FunctionCallOperatorNode : public OperatorNode {
    std::unique_ptr<Node> subject_; // note, args_ only contains things in (...)
    optional_ref<const type::FunctionNode> signature_;
    optional_ref<const symbol::Symbol> symbol_; // populated if calling a symbol

  public:
    /**
     * @brief Construct a function call expression.
     * @param token Start token of the expression.
     * @param symbol Token marking the call (opening parenthesis).
     * @param subject Callee expression.
     * @param args Call arguments.
     */
    FunctionCallOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> subject, std::deque<std::unique_ptr<Node>> args);

    /** @brief Return the node kind name, "function call". */
    std::string node_name() const override { return "function call"; }

    /**
     * @brief Print the callee, arguments, and this call in tree form.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_tree(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this call as `subject(arg1, arg2, ...)` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Always true: a function call may write its result to `$ret`. */
    bool writes_to_ret() const override { return true; }

    /**
     * @brief Collect symbols from the callee and each argument.
     * @param messages Message list to report errors into.
     * @param registry Registry to collect symbols into.
     * @return True on success.
     */
    bool collate_registry(message::List &messages, symbol::Registry &registry) override;

    /**
     * @brief Process the callee and arguments, resolving the call to a concrete function symbol or operator overload.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code evaluating the callee/arguments and emitting the call.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };

  /** @brief Represents `sizeof expr` or `sizeof <type>`, resolving to the byte size of the operand's type or a directly given type. */
  class SizeOfOperatorNode : public OperatorNode {
    optional_ref<const type::Node> type_; // expr_ may be nullptr

  public:
    /**
     * @brief Construct a `sizeof expr` expression.
     * @param token Start token of the expression.
     * @param symbol Token of the `sizeof` keyword.
     * @param expr Expression whose type's size is taken.
     */
    SizeOfOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> expr);

    /**
     * @brief Construct a `sizeof <type>` expression.
     * @param token Start token of the expression.
     * @param symbol Token of the `sizeof` keyword.
     * @param type Type whose size is taken directly.
     */
    SizeOfOperatorNode(lexer::Token token, lexer::Token symbol, const type::Node& type);

    /**
     * @brief Print this expression as `sizeof(expr)` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Process this node, fixing its value's type to `uint64`.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code loading the constant size of the operand's type (or the directly given type).
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };

  /** @brief Represents `lhs[rhs]`, either pointer/array subscripting or an overloaded `operator[]`. */
  class SubscriptOperatorNode : public OperatorNode {
    optional_ref<const type::FunctionNode> signature_;
    optional_ref<const ops::Operator> op_; // overloaded operator, in case of non-pointer behaviour

  public:
    /**
     * @brief Construct a subscript expression.
     * @param token Start token of the expression.
     * @param symbol Token of the `[` bracket.
     * @param lhs Expression being indexed.
     * @param rhs Index expression.
     */
    SubscriptOperatorNode(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    /**
     * @brief Print this expression as `lhs[rhs]` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /** @brief Test whether the resolved operator is user-defined (and so may write to `$ret`), or any operand may. */
    bool writes_to_ret() const override;

    /**
     * @brief Process both operands, choosing between pointer/array indexing and overload resolution based on the LHS's type.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code for pointer/array indexing, or invoke the resolved `operator[]` overload.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };

  /** @brief Represents short-circuiting `&&`/`||`, distinct from ops::LazyLogicalOperator, the corresponding builtin-operator invoked at runtime. */
  class LazyLogicalOperator : public OperatorNode {
    bool and_; // && or ||

  public:
    /**
     * @brief Construct a lazy logical expression.
     * @param token Start token of the expression.
     * @param symbol Token of the `&&`/`||` operator.
     * @param lhs Left-hand operand.
     * @param rhs Right-hand operand.
     */
    LazyLogicalOperator(lexer::Token token, lexer::Token symbol, std::unique_ptr<Node> lhs, std::unique_ptr<Node> rhs);

    /**
     * @brief Process both operands, requiring each to have a boolean-compatible type.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool process(Context &ctx) override;

    /**
     * @brief Generate code invoking the matching builtin logical operator, which performs the short-circuit branching.
     * @param ctx Compilation context.
     * @return True on success.
     */
    bool generate_code(Context &ctx) override;
  };
}
