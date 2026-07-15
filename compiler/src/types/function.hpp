#pragma once

#include <deque>
#include "node.hpp"
#include "unit.hpp"
#include "optional_ref.hpp"

namespace lang::type {
  /**
   * @brief Represents a function *signature*, distinct from ast::FunctionNode, the AST definition node.
   *
   * Stores an ordered list of parameter type references and a return type
   * (defaulting to unit). Provides overload-resolution helpers (`filter_candidates`)
   * that score candidate signatures by parameter-type match, and a caching
   * factory (`create`) that interns signatures in the global TypeGraph.
   */
  class FunctionNode : public Node {
    std::deque<std::reference_wrapper<const Node>> parameters_;
    const Node& returns_;

  public:
    /**
     * @brief Construct a function signature returning unit.
     * @param parameters Parameter types, in order.
     */
    explicit FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters) : parameters_(std::move(parameters)), returns_(unit) {}

    /**
     * @brief Construct a function signature with an explicit return type.
     * @param parameters Parameter types, in order.
     * @param returns Return type.
     */
    FunctionNode(std::deque<std::reference_wrapper<const Node>> parameters, const Node& returns) : parameters_(std::move(parameters)), returns_(returns) {}

    /** @brief Return the node kind name, "function". */
    std::string node_name() const override { return "function"; }

    /** @brief Return the number of parameters. */
    size_t args() const { return parameters_.size(); }

    /**
     * @brief Return the type of the ith parameter.
     * @param i Index of the parameter.
     * @return The parameter's type.
     */
    const Node& arg(int i) const { return parameters_[i]; }

    /** @brief Return the return type. */
    const Node& returns() const { return returns_; }

    /** @brief Return this node, since it is already a FunctionNode. */
    const FunctionNode* get_func() const override { return this; }

    /**
     * @brief Print this signature as `(params) -> return` source code.
     * @param os Output stream to write to.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, unsigned int indent_level = 0) const override;

    /**
     * @brief Print this signature's parameter list, optionally including the return type.
     * @param os Output stream to write to.
     * @param print_return Whether to print the `-> return` suffix.
     * @param indent_level Current indentation depth.
     * @return The same stream, for chaining.
     */
    std::ostream& print_code(std::ostream &os, bool print_return, unsigned int indent_level = 0) const;

    /** @brief Always 8: functions are referenced via an address. */
    size_t size() const override { return 8; } // size of an address

    /** @brief Always the unsigned 64-bit assembly datatype (functions are handled as addresses). */
    constants::inst::datatype::dt get_asm_datatype() const override;

    /** @brief Always false in the general case (the global-label edge case is handled elsewhere). */
    bool reference_as_ptr() const override { return false; }

    /** @brief Return the label representation, concatenating each parameter type's label. */
    std::string to_label() const override;

    /**
     * @brief Get or create the interned function signature matching the given parameters and (optionally) return type.
     *
     * If `returns` is provided it is also used in the match; if no existing
     * signature matches, a new one is created with the given return type
     * (or unit if none was provided).
     * @param parameters Parameter types, in order.
     * @param returns Return type to match/use, if constrained.
     * @return Reference to the matching or newly created signature.
     */
    static const FunctionNode& create(const std::deque<std::reference_wrapper<const Node>>& parameters, optional_ref<const Node> returns);

    /**
     * @brief Filter a list of candidate signatures to those best matching this signature's parameter types.
     * @param options Candidate signatures to filter.
     * @return The best-matching candidates (more than one indicates ambiguity).
     */
    std::deque<std::reference_wrapper<const FunctionNode>> filter_candidates(const std::deque<std::reference_wrapper<const FunctionNode>>& options) const;

    /**
     * @brief Filter a list of candidate signatures to those best matching a given list of argument types.
     *
     * Scores each viable candidate (matching arity, all parameters equal or
     * supertypes) by counting exact type matches, then keeps only the
     * candidates achieving the maximum score.
     * @param parameters Argument types to match against.
     * @param options Candidate signatures to filter.
     * @return The best-matching candidates (more than one indicates ambiguity).
     */
    static std::deque<std::reference_wrapper<const FunctionNode>> filter_candidates(const std::deque<std::reference_wrapper<const Node>>& parameters, const std::deque<std::reference_wrapper<const FunctionNode>>& options);
  };
}
