#pragma once

#include <string>
#include <memory>
#include <deque>
#include "optional_ref.hpp"
#include "control_flow/conditional_context.hpp"
#include "memory/storage_location.hpp"

namespace message {
  class MessageGenerator;
  class List;
}

namespace lang {
  struct Context;

  namespace ast {
    class Node;
  }

  namespace type {
    class FunctionNode;
  }

  namespace value {
    class Value;
  }
}

namespace lang::ops {
  using OperatorId = unsigned int;

  /**
   * @brief Call-site context passed to Operator::invoke, carrying an optional branch-fusion target and the invocation's source location.
   */
  struct InvocationOptions {
    optional_ref<control_flow::ConditionalContext> conditional;
    Location origin;
  };

  /**
   * @brief Abstract base identifying one resolvable operator overload, built-in or user-defined, by a unique id, its textual symbol, and its function signature.
   */
  class Operator {
    OperatorId id_;
    std::string op_;
    const type::FunctionNode& type_;

  public:
    /**
     * @brief Constructs the identity of an operator overload, assigning it a fresh globally-unique id.
     * @param symbol Textual operator symbol (e.g. "+", "[]").
     * @param type Function signature this overload matches against.
     */
    Operator(std::string symbol, const type::FunctionNode& type);

    virtual ~Operator() = default;

    /**
     * @brief Returns the operator's unique id.
     * @return The id.
     */
    OperatorId id() const { return id_; }

    /**
     * @brief Returns the operator's textual symbol.
     * @return The symbol.
     */
    const std::string& op() const { return op_; }

    /**
     * @brief Returns the operator's function signature.
     * @return The signature.
     */
    const type::FunctionNode& type() const { return type_; }

    /**
     * @brief Writes a human-readable declaration of this operator overload (symbol plus signature).
     * @param os Output stream to write to.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream& os) const;

    /**
     * @brief Generates code performing this operator's call, depositing the result in an output value.
     * @param ctx Compiler context.
     * @param args Argument AST nodes, not yet necessarily code-generated.
     * @param return_value Output value populated with the result.
     * @param options Call-site context, including an optional branch-fusion target.
     * @return True on success, false if code generation failed.
     */
    // invoke the given operator
    virtual bool invoke(Context& ctx, const std::deque<std::unique_ptr<ast::Node>>& args, value::Value& return_value, const InvocationOptions& options) const = 0;

    /**
     * @brief Reports whether this is a compiler-builtin operator, as opposed to a user-defined overload.
     * @return True if builtin.
     */
    // are we built-in or overloaded
    virtual bool builtin() const = 0;
  };

  /**
   * @brief Looks up every registered operator sharing a given symbol, regardless of signature.
   * @param symbol Operator symbol to look up.
   * @return Matching operators, or an empty deque if none.
   */
  // get a list of references of operators with this name
  std::deque<std::reference_wrapper<const Operator>> get(const std::string& symbol);

  /**
   * @brief Looks up the single registered operator matching both symbol and signature exactly.
   * @param symbol Operator symbol to look up.
   * @param type Exact function signature to match.
   * @return The matching operator, or nothing if none matches.
   */
  // get a reference to the given operator, return None if it does not exist
  std::optional<std::reference_wrapper<const Operator>> get(const std::string& symbol, const type::FunctionNode& type);

  /**
   * @brief Registers a newly constructed operator into the global registry, taking ownership.
   * @param op Operator to register.
   */
  // add operator to store
  void store_operator(std::unique_ptr<Operator> op);

  /**
   * @brief Resolves the single best-matching registered operator for a symbol and argument signature, or reports a diagnostic.
   * @param symbol Operator symbol being called.
   * @param signature Signature the call site's arguments produce.
   * @param source Location to attribute any diagnostics to.
   * @param messages Message list to append diagnostics to.
   * @return The resolved operator, or empty if no candidate or multiple ambiguous candidates were found.
   */
  // try to find the given operator, generating an error if not
  optional_ref<const Operator> select_candidate(const std::string& symbol, const type::FunctionNode& signature, const message::MessageGenerator& source, message::List& messages);
}
